/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "clogdatasource.h"


#include <thread>
#include <chrono>

#if defined(_WIN32) || defined(_WIN64)
#   define NOMINMAX
#   include <Windows.h>
#else
#   include <dirent.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#endif

#if 1 || __cplusplus > 201103L
using namespace std::chrono_literals;
#else
constexpr std::chrono::seconds operator "" s(unsigned long long x_val)
{
    return (std::chrono::seconds(x_val));
}

constexpr std::chrono::milliseconds operator "" ms(unsigned long long x_val)
{
    return (std::chrono::milliseconds(x_val));
}
#endif

#ifndef LOG_ERROR_A
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define LOG_ERROR_A(mess) qFatal("%s", mess)
#   define LOG_INFO_A(mess) qInfo("%s", mess)
#endif // LOG_ERROR_A

#if 0
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

CLogDatasource::CLogDatasource()
{
    m_mapNewlines.resize(m_newlineBufferSize, 0);
    m_tempLineBuffer.resize(m_const_max_string_size, 0);
}

CLogDatasource::~CLogDatasource()
{
    shutdown();
}

void CLogDatasource::shutdown()
{
    m_callbackUpdate = nullptr;
    m_abort = true;
    m_conditionVariable_sleepTimer.notify_all();

    // wait for shutdown all active threads
    auto timeout = 1000;
    while (m_threadActive && timeout--)
        std::this_thread::sleep_for(20ms);
}

void CLogDatasource::open(const QString &path, const std::function<void(void)> &callbackUpdate)
{

    m_callbackUpdate = callbackUpdate;
    if(callbackUpdate == nullptr)
        return;

    m_fileRead.setFileName(path);
    if (!m_fileRead.open(QIODevice::ReadOnly))
        throw std::runtime_error(m_fileRead.errorString().toStdString());

    m_fileWatch.setFileName(path);
    if (!m_fileWatch.open(QIODevice::ReadOnly))
        throw std::runtime_error(m_fileWatch.errorString().toStdString());

    auto runWatchThread = [](CLogDatasource* pThis)
    {
        pThis->WatchThread();
    };
    (std::thread(runWatchThread, this)).detach();



}

void CLogDatasource::WatchThread()
{
    m_threadActive = true;
    while (!m_abort)
    {
        try
        {
            WatchThread2();
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_A(e.what());
        }
        catch (...)
        {
            try// c++xx >= c++11
            {
                std::exception_ptr curr_excp = std::current_exception();
                if (curr_excp)
                    std::rethrow_exception(curr_excp);
                LOG_ERROR_A("WatchThread: Unexpected exception");
            }
            catch (const std::exception& e)
            {
                LOG_ERROR_A(e.what());
            }
        }
    }
    m_threadActive = false;
}

void CLogDatasource::WatchThread2()
{
    DEBUGTRACE();

    constexpr auto symbolMaxSize = sizeof(uint32_t);
    constexpr auto sectorSize = 512;
    constexpr auto multiplerSize = 500;
    constexpr int32_t bufferSize = symbolMaxSize * sectorSize * multiplerSize;

    std::vector<uint8_t> buffer(bufferSize);
    auto bufferPtr = buffer.data();

    bool bUtf32 = false;
    bool bUtf16 = false;

    int64_t lastFileSize = 0;

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
    auto fname = m_fileWatch.fileName().toStdWString();
#else
    auto fname = m_fileWatch.fileName().toStdString();
#endif

    while (!m_abort)
    {
        // read from 0 position, check Unicode signature
        if (lastFileSize == 0)
        {

            static_assert(sizeof(buffer[0]) == 1, "signatureSize error");
            constexpr auto signatureSize = 4;

            m_fileWatch.seek(0);
            auto res = m_fileWatch.read(reinterpret_cast<char*>(bufferPtr), signatureSize);
            if (res != signatureSize)
            {
                std::unique_lock<std::mutex> locker(m_mutex_sleepTimer);
                m_conditionVariable_sleepTimer.wait_for(locker, 500ms);
                continue;
            }

            bUtf32 = (bufferPtr[0] == 0x00 && bufferPtr[1] == 0x00 && bufferPtr[2] == 0xFE && bufferPtr[3] == 0xFF) ||
                     (bufferPtr[0] == 0xFF && bufferPtr[1] == 0xFE && bufferPtr[2] == 0x00 && bufferPtr[3] == 0x00);

            bUtf16 = !bUtf32 && ((bufferPtr[0] == 0xFF && bufferPtr[1] == 0xFE) ||
                                 (bufferPtr[0] == 0xFE && bufferPtr[1] == 0xFF));

            m_bUtf32 = bUtf32;
            m_bUtf16 = bUtf16;

            m_fileWatch.seek(m_bUtf32 ? 4 : m_bUtf16 ? 2 : 0);
            m_rowsInFile = 0;
        }

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
        struct __stat64 flstat;
        auto retcode = _wstat64( fname.c_str(), &flstat);
#else
        struct stat64 flstat{};
        auto retcode = stat64(fname.c_str(), &flstat);
#endif
        int64_t fSize;
        if (retcode == 0)
        {
            fSize = flstat.st_size;
        }
        else // error
        {
            fSize = m_fileWatch.size();
        }

        //reset if the file is new or truncated
        if (fSize < lastFileSize)
        {
            lastFileSize = 0;
            m_rowsInFile = 0;
            //            qDebug() << "  onXrowCountChanged  fSize[" << fSize << "] < lastFileSize [" << lastFileSize << "] "  ;
            m_callbackUpdate();
            continue;
        }

        if (lastFileSize == fSize)
        {
            std::unique_lock<std::mutex> locker(m_mutex_sleepTimer);
            m_conditionVariable_sleepTimer.wait_for(locker, 500ms);
            continue;
        }

        // read and calculate new lines in a new file fragment
        //        qDebug() << fSize;
        lastFileSize = fSize;
        if (bUtf32)
            calcLineCountInFile(reinterpret_cast<uint32_t*>(bufferPtr), bufferSize);
        else if (bUtf16)
            calcLineCountInFile(reinterpret_cast<uint16_t*>(bufferPtr), bufferSize);
        else
            calcLineCountInFile(reinterpret_cast<uint8_t *>(bufferPtr), bufferSize);
    }
}
void CLogDatasource::addPointer(int32_t index, int64_t startOffset)
{
    {
        std::lock_guard<std::mutex> locker(m_mutex_mapUpdate);
        if (index + 1 >= static_cast<int32_t>(m_mapNewlines.size()))
            m_mapNewlines.resize(m_mapNewlines.size() + m_newlineBufferSize);
    }
    m_mapNewlines[static_cast<uint32_t>(index)] = startOffset;
}

template<typename T>
inline void CLogDatasource::calcLineCountInFile(T * ptr, int32_t bytesCount)
{

    constexpr auto bytesPerChar = static_cast<int8_t>(sizeof(T));

    do
    {
        int32_t charsToRead;

        const auto startOffset = m_fileWatch.pos();
        const auto readBytes = m_fileWatch.read(reinterpret_cast<char*>(ptr), bytesCount);
        if (readBytes < bytesPerChar) // no more data, return
        {
            m_fileWatch.seek(startOffset);
            return;
        }

        if (bytesPerChar != 1)
        {
            charsToRead = static_cast<int32_t>(readBytes) / bytesPerChar;
            auto remain = static_cast<int32_t>(readBytes) % bytesPerChar;
            if (remain)
            {
                const auto pos = m_fileWatch.pos() - remain;
                m_fileWatch.seek(pos);
            }
        }
        else
        {
            charsToRead = static_cast<int32_t>(readBytes);
        }

        auto lineCount = m_rowsInFile.load();

        if(lineCount == 0) // write first index (start line)
        {
            // [0] start line
            // [1] end line
            auto tmpPtr = startOffset;
            addPointer(lineCount, tmpPtr);
            lineCount++;
            m_openLine = lineCount; // set flag to incomplete line
        }

        int bAddLine = 0;
        for (int32_t ind = 0; ind < charsToRead; ind++)
        {
            bool bLf = ptr[ind] == 0xD || ptr[ind] == 0xA;
            if (bLf) // new line sequence.
            {
                if(bAddLine == 0)
                {
                    auto tmpPtr = startOffset + (static_cast<int64_t>(ind) * bytesPerChar);
                    if(m_openLine != -1)
                    {
                        addPointer(m_openLine, tmpPtr);
                        m_openLine = -1; // set flag indicate complete line
                    }
                    else
                    {
                        addPointer(lineCount, tmpPtr);
                        lineCount++;
                    }
                }
                bAddLine++;
                continue;
            }
            bAddLine = 0;
            if(m_openLine == -1)
            {
                lineCount++;
                m_openLine = lineCount; // set flag to incomplete line
            }
        }

        if(m_openLine != -1)// update incomplete line
        {
            auto tmpPtr = startOffset + (static_cast<int64_t>(charsToRead) * bytesPerChar);
            addPointer(m_openLine, tmpPtr);
        }

        if (m_rowsInFile.load() != lineCount)
            m_rowsInFile = lineCount;

        //        qDebug() << "     onXrowCountChanged  m_rowsInFile[" << m_rowsInFile.load() << "] ";
        for (int ind = 0; ind < m_rowsInFile.load();ind++ ) {

        }
        m_callbackUpdate();

    } while (!m_abort);
}

QString CLogDatasource::getLogLine(int64_t indRow) const
{

    constexpr uint32_t maxRead = m_const_max_string_size - 8;

    auto availableLines = m_rowsInFile.load();
    if (availableLines <= indRow)
        return QString();

    if (!m_fileRead.isOpen())
        return QString();

    int64_t offsetStart = 0;
    int64_t offsetEnd = -1;
    {
        std::lock_guard<std::mutex> locker(m_mutex_mapUpdate);
        offsetStart = m_mapNewlines[static_cast<uint32_t>(indRow)];
        indRow++;
        offsetEnd = m_mapNewlines[static_cast<uint32_t>(indRow)];
    }

    if (offsetEnd <= offsetStart)
        return QString();

    if (!m_fileRead.seek(offsetStart))
        return QString();

    auto stringByteLength = static_cast<uint32_t>(offsetEnd - offsetStart);
    stringByteLength = std::min<uint32_t>(stringByteLength, maxRead);

    auto ptr = reinterpret_cast<const char*>(m_tempLineBuffer.data());
    auto res = m_fileRead.read(const_cast<char*>(ptr), stringByteLength);
    if (res < 1)
        return QString();

    if (m_bUtf32)
    {
        auto charCount = static_cast<char32_t>(res) / static_cast<char32_t>(sizeof(char32_t));
        return QString::fromUcs4(reinterpret_cast<const char32_t*>(ptr), charCount).replace('\n', "").replace('\r', "");
    }
    else if (m_bUtf16)
    {
        auto charCount = static_cast<char32_t>(res) / static_cast<char32_t>(sizeof(char16_t));
        return QString::fromUtf16(reinterpret_cast<const char16_t*>(ptr), charCount).replace('\n', "").replace('\r', "");
    }
    else
    {
        auto charCount = static_cast<int32_t>(res);
        return QString::fromUtf8(reinterpret_cast<const char*>(ptr), charCount).replace('\n', "").replace('\r', "");
    }
}

int64_t CLogDatasource::getLogLineCount() const
{
    return m_rowsInFile.load();
}
