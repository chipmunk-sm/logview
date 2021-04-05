/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CLOGDATASOURCE_H
#define CLOGDATASOURCE_H

#include <QFile>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <string>

class CLogDatasource
{
public:
    constexpr static uint32_t m_newlineBufferSize = 4 * 1024; // 4 Kilobytes
    constexpr static uint32_t m_const_max_string_size = 8192;

    CLogDatasource();
    ~CLogDatasource();

    QString getLogLine(int64_t indRow) const;
    int64_t getLogLineCount() const;

    void open(const QString &path, const std::function<void ()> &callbackUpdate);

    void shutdown();
private:
    std::function<void(void)> m_callbackUpdate;
    template <typename T>
    void calcLineCountInFile(T *ptr, int32_t bytesCount);
    void WatchThread();
    void WatchThread2();
    void addPointer(int32_t index, int64_t startOffset);


    mutable QFile            m_fileRead;
    QFile                    m_fileWatch;
    std::atomic<int32_t>     m_rowsInFile = 0;

    std::condition_variable  m_conditionVariable_sleepTimer;
    std::mutex               m_mutex_sleepTimer;
    bool                     m_abort = false;
    bool                     m_threadActive = false;

    std::vector<int64_t>     m_mapNewlines;
    std::vector<uint8_t>     m_tempLineBuffer;
    bool                     m_bUtf16 = false;
    bool                     m_bUtf32 = false;
    mutable std::mutex       m_mutex_mapUpdate;
    int32_t                  m_openLine = -1;


};

#endif // CLOGDATASOURCE_H
