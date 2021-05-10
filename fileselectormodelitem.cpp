/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselectorlocations.h"
#include "fileselectormodelitem.h"

#if defined(_WIN32) || defined(_WIN64)
#   define NOMINMAX
#   include <Windows.h>
#else
#   include <dirent.h>
#endif

#   include <sys/stat.h>
#   include <sys/types.h>

#if !defined(S_ISDIR)
#   define S_ISDIR(X) (((X) & _S_IFDIR) == _S_IFDIR)
#endif

#if !defined(S_ISREG)
#   define S_ISREG(mode) (((mode) & _S_IFREG) == _S_IFREG)
#endif

#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <ctime>

#include <thread>
#include <algorithm>

//#define USESTDFILESYSTEM

#if defined(USESTDFILESYSTEM)
#   include <filesystem>
#endif // defined(USESTDFILESYSTEM)

#ifndef LOG_ERROR_A
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define LOG_ERROR_A(mess) qFatal("%s", mess)
#   define LOG_WARNING_A(mess) qWarning("%s", mess)
#endif // LOG_ERROR_A

#if 0
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif


#if defined(_WIN32) || defined(_WIN64)
QDateTime DatetimeFromFiletime(const FILETIME &filetime)
{
    if (filetime.dwHighDateTime == 0 && filetime.dwLowDateTime == 0)
        return QDateTime();

    SYSTEMTIME sTime;
    FileTimeToSystemTime(&filetime, &sTime);
    return QDateTime(QDate(sTime.wYear, sTime.wMonth, sTime.wDay),
        QTime(sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds),
        Qt::UTC);
}

QDateTime DatetimeFromFiletime(int64_t filetime)
{
    FILETIME ft;
    ft.dwLowDateTime = static_cast<DWORD>(filetime & 0xFFFFFFFF);
    ft.dwHighDateTime = static_cast<DWORD>(filetime >> 32);
    return DatetimeFromFiletime(ft);
}
#else

#endif

FileSelectorModelItem::FileSelectorModelItem(FileSelectorModelItem *parentItem, eLocations m_locationId)
    : m_parentItem(parentItem)
    , m_locationId(m_locationId)
{
}

FileSelectorModelItem::~FileSelectorModelItem()
{
    qDeleteAll(m_childItems);
}

int FileSelectorModelItem::columnCount() const
{
    return 4;
}

FileSelectorModelItem *FileSelectorModelItem::appendChild(eLocations locationId)
{
    auto itemPtr = new FileSelectorModelItem(this);
    itemPtr->m_itemName = LocationsEnum::GetLocationName(locationId);
    itemPtr->m_locationId = locationId;
    m_childItems.emplace_back(itemPtr);
    return itemPtr;
}

void FileSelectorModelItem::appendChild(const QString &name, const QString &path)
{
#if  defined(_WIN32) || defined(_WIN64)
    struct _stat64 flstat{};
    if (_wstati64(path.toStdWString().c_str(), &flstat) != 0)
        return;
#else
    struct stat64 flstat{};
    if (stat64(path.toStdString().c_str(), &flstat) != 0)
        return;
#endif
    appendChild(name, path, S_ISDIR(flstat.st_mode), flstat.st_size, flstat.st_mtime);
}

void FileSelectorModelItem::appendChild(const QString &name, const QString &path, const bool isDir, const int64_t &fileSizeBytes, const int64_t &filetime)
{

    for (decltype(m_childItems.size()) index = 0; index < m_childItems.size(); index++) {
        if(m_childItems[index]->m_itemName == name && m_childItems[index]->m_isDir == isDir) {
            auto itemPtr = m_childItems[index];
            itemPtr->m_state = (itemPtr->m_fileSizeBytes != fileSizeBytes || itemPtr->m_filetime != filetime) ?
                                FileSelectorModelItem::StateType::StateType_update : FileSelectorModelItem::StateType::StateType_valid;
            itemPtr->m_fileSizeBytes = fileSizeBytes;
            itemPtr->m_filetime = filetime;
            return;
        }
    }

    auto itemPtr = new FileSelectorModelItem(this);
    itemPtr->m_itemName = name;
    itemPtr->m_Path = path;
    itemPtr->m_isDir = isDir;
    itemPtr->m_fileSizeBytes = fileSizeBytes;
    itemPtr->m_filetime = filetime;
    itemPtr->m_state = FileSelectorModelItem::StateType::StateType_new;
    m_childItems.emplace_back(itemPtr);
}

QString FileSelectorModelItem::bytesToString(int64_t val, int64_t base) const
{
    QString result;
    const auto bytesCount = static_cast<double>(val);
    auto kBytes = static_cast<double>(base);
    auto mBytes = kBytes * kBytes;
    auto gBytes = kBytes * mBytes;

    if (static_cast<decltype(kBytes)>(val) < kBytes)
        result = val == 0 ? QString("  0 Bytes  ") : QString("  %1 Bytes  ").arg(val);
    else if (static_cast<decltype(mBytes)>(val) < mBytes)
        result = QString("  %1 KB  ").arg(bytesCount / kBytes, 0, 'f', 2);
    else if (static_cast<decltype(gBytes)>(val) < gBytes)
        result = QString("  %1 MB  ").arg(bytesCount / mBytes, 0, 'f', 2);
    else
        result = QString("  %1 GB  ").arg(bytesCount / gBytes, 0, 'f', 2);

    return result;
}

FileSelectorModelItem *FileSelectorModelItem::child(int row) const
{

    if (row < 0 || row >= static_cast<int32_t>(m_childItems.size()))
        return nullptr;
    return m_childItems[static_cast<uint32_t>(row)];
}

int FileSelectorModelItem::fetchChild(const QModelIndex &parent)
{

    if (getLocationId() == eLocations::eLocations_folder_recent)
    {
        std::vector<std::pair<eLocations, std::vector<QString>>> locationsList;
        FileSelectorLocations::GetRecentLocations(locationsList);
        for (int index = 0; index < static_cast<int>(m_childItems.size()); index++)
            m_childItems[static_cast<uint>(index)]->setState(FileSelectorModelItem::StateType::StateType_unknown);
        if(locationsList.size() > 0)
        {
            const auto & xRecentList = locationsList.front().second;
            for (const auto & tmpName : xRecentList) {
                appendChild(tmpName, tmpName);
            }
        }
        testChild(parent);
        return static_cast<int>(m_childItems.size());
    }

    if (getLocationId() != eLocations::eLocations_None)
        return static_cast<int>(m_childItems.size());

    if (!m_isDir)
        return 0;

    const auto basePath = getPath();

    if (basePath.isEmpty())
        return static_cast<int>(m_childItems.size());

    time_t nowtime = time(nullptr);
    auto diffsec = nowtime - m_lasttime;

    if (m_lasttime != 0 && diffsec < 2)
    {
        m_lasttime = nowtime;
        return static_cast<int>(m_childItems.size());
    }

    m_lasttime = nowtime;


//#define SPEEDTEST
#ifdef SPEEDTEST
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif // SPEEDTEST

    for (int index = 0; index < static_cast<int>(m_childItems.size()); index++)
        m_childItems[static_cast<uint>(index)]->setState(FileSelectorModelItem::StateType::StateType_unknown);

#if defined(USESTDFILESYSTEM)

//#error std::filesystem for android keep fuck-up prize? 2021 need retest

    try
    {
#if defined(_WIN32) || defined(_WIN64)
        const auto path = basePath.toStdWString();
#else
        const auto path = basePath.toStdString();
#endif
        if (std::filesystem::exists(path))
        {
            for (const auto & fd : std::filesystem::directory_iterator(path))
            {
#if defined(_WIN32) || defined(_WIN64)
                const auto itemName = QString::fromWCharArray(fd.path().filename().wstring().c_str());
                const auto itemPath = QString::fromWCharArray(fd.path().wstring().c_str());
#else
                const auto itemName = QString::fromUtf8(fd.path().filename().string().c_str());
                const auto itemPath = QString::fromUtf8(fd.path().string().c_str());
#endif
                appendChild(itemName,
                            itemPath,
                            fd.is_directory(),
                            static_cast<int64_t>(fd.file_size()),
                            static_cast<int64_t>(fd.last_write_time().time_since_epoch().count()));
            }
        }
    }
    catch (...)
    {
          return static_cast<int>(m_childItems.size());
    }

#elif defined(_WIN32) || defined(_WIN64)

    const auto path = (basePath.back() == QLatin1Char('\\') || basePath.back() == QLatin1Char('/')) ?
                (basePath + QLatin1String("*")).toStdWString() : (basePath + QLatin1String("\\*")).toStdWString();

    WIN32_FIND_DATAW fd;
    auto hFind = ::FindFirstFileW(path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do{
            if ((wcsncmp(L".", fd.cFileName, 1) == 0) || (wcsncmp(L"..", fd.cFileName, 2) == 0))
                continue;

            const bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;

            LARGE_INTEGER filesize;
            filesize.LowPart = fd.nFileSizeLow;
            filesize.HighPart = static_cast<LONG>(fd.nFileSizeHigh);
            const int64_t fileSizeBytes = static_cast<int64_t>(filesize.QuadPart);
            const int64_t filetime = (static_cast<int64_t>(fd.ftLastWriteTime.dwHighDateTime) << 32) + fd.ftLastWriteTime.dwLowDateTime;

            const QString itemName = QString::fromWCharArray(fd.cFileName);
            const QString itemPath = QDir::toNativeSeparators(basePath + QLatin1Char('/') + itemName);

            appendChild(itemName, itemPath, isDir, fileSizeBytes, filetime);

        } while (::FindNextFileW(hFind, &fd) != 0);

        ::FindClose(hFind);
    }

#else // UNIX

    DIR *dir = ::opendir(basePath.toStdString().c_str());
    if (dir != nullptr)
    {
        struct dirent *dirEntry;
        while ((dirEntry = ::readdir(dir)) != nullptr)
        {
            if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)
                continue;

            const auto itemName = QString::fromUtf8(dirEntry->d_name);
            const auto itemPath = QDir::toNativeSeparators(QDir(basePath + QLatin1Char('/') + itemName).absolutePath());
            appendChild(itemName, itemPath);
        }
        closedir(dir);
    }

#endif // UNIX

    testChild(parent);

#ifdef SPEEDTEST

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    qDebug().noquote() << "fetch << \t" << duration.count() << "\titems [" << static_cast<int32_t>(m_childItems.size()) << "]\t[" << getPath() << "]";

#endif // SPEEDTEST

    return static_cast<int>(m_childItems.size());
}

void FileSelectorModelItem::SortChild(FileSelectorModelItem::SortType sortType)
{
    std::vector<FileSelectorModelItem *> &childItems = m_childItems;
    QCollator coll;
#if defined (Q_OS_ANDROID)
    coll.setNumericMode(false);
#else
    coll.setNumericMode(true);
#endif
    std::sort(childItems.begin(), childItems.end(), [&coll, &sortType](const FileSelectorModelItem *s1, const FileSelectorModelItem *s2)
    {
        switch (sortType)
        {
        case SortType::SortType_name:
        {
            return coll.compare(s1->getItemName(), s2->getItemName()) < 0;
        }
        case SortType::SortType_name_dirFirst:
        {
            bool left = s1->m_isDir;
            bool right = s2->m_isDir;
            if (left ^ right)
                return left;

            return coll.compare(s1->getItemName(), s2->getItemName()) < 0;
        }
        case SortType::SortType_size:
        {
            bool left = s1->m_isDir;
            bool right = s2->m_isDir;
            if (left ^ right)
                return left;

            auto sizeDifference = s2->getFileSizeBytes() - s2->getFileSizeBytes();
            if (sizeDifference == 0)
                return coll.compare(s1->getItemName(), s2->getItemName()) < 0;

            return sizeDifference < 0;
        }
        case SortType::SortType_type:
        {
            bool left = s1->m_isDir;
            bool right = s2->m_isDir;
            if (left ^ right)
                return left;

            int compare = coll.compare(s1->getItemExtension(), s2->getItemExtension());
            if (compare == 0)
                return coll.compare(s1->getItemName(), s2->getItemName()) < 0;

            return compare < 0;
        }
        case SortType::SortType_date:
        {
            auto s1M = s1->getFiletime();
            auto s2M = s2->getFiletime();
            if (s1M == s2M)
                return coll.compare(s1->getItemName(), s2->getItemName()) < 0;

            return s1M < s2M;
        }
        }
        return false;
    });
}

QString FileSelectorModelItem::getItemName() const
{
    return m_itemName;
}

QString FileSelectorModelItem::getItemSize() const
{
    if (m_isDir)
    {
        const auto items = m_childItems.size();
        if(items == 1)
            return QString("  1 item  ");
        return QString("  %1 items  ").arg(items);
    }

    return bytesToString(getItemSizeBytes());
}

int64_t FileSelectorModelItem::getItemSizeBytes() const
{
#if  defined(_WIN32) || defined(_WIN64)
    struct _stat64 flstat{};
    if (_wstati64(m_Path.toStdWString().c_str(), &flstat) != 0)
        return 0;
    return flstat.st_size;
#else
    struct stat64 flstat{};
    if (stat64(m_Path.toStdString().c_str(), &flstat) != 0)
        return 0;
    return flstat.st_size;
#endif
}

QString FileSelectorModelItem::getItemType() const
{
    if (m_locationId != eLocations::eLocations_None)
        return QObject::tr("  Bookmark  ");
    return m_isDir ? QObject::tr("  Folder  ") : QObject::tr("  File  ");
}

QString FileSelectorModelItem::getItemDate() const
{
#if  defined(_WIN32) || defined(_WIN64)
    struct _stat64 flstat{};
    if (_wstati64(m_Path.toStdWString().c_str(), &flstat) != 0)
        return QString();
#else
    struct stat64 flstat{};
    if (stat64(m_Path.toStdString().c_str(), &flstat) != 0)
        return QString();
#endif
    QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<uint>(flstat.st_mtime));

    return QLatin1String("  ") +
            QLocale().toString(dt, QLocale::ShortFormat) +
            QLatin1String("  ");
}

QString FileSelectorModelItem::getPath() const
{
    return m_Path;
}

FileSelectorModelItem *FileSelectorModelItem::getParentItem() const
{
    return m_parentItem;
}

bool FileSelectorModelItem::isDir() const
{
    return m_isDir;
}

int64_t FileSelectorModelItem::getFileSizeBytes() const
{
    return m_fileSizeBytes;
}

int64_t FileSelectorModelItem::getFiletime() const
{
    return m_filetime;
}

QString FileSelectorModelItem::getItemExtension() const
{
    auto ind = m_itemName.lastIndexOf('.');
    auto maxSlash = std::max(m_itemName.lastIndexOf('\\'), m_itemName.lastIndexOf('/'));
    if (ind < 0 || maxSlash > static_cast<int32_t>(ind))
        return "";
    return m_itemName.mid(++ind);
}

eLocations FileSelectorModelItem::getLocationId() const
{
    return m_locationId;
}

void FileSelectorModelItem::testChild(const QModelIndex &parent)
{

    if (getLocationId() != eLocations::eLocations_folder_recent)
        SortChild(FileSelectorModelItem::SortType::SortType_name_dirFirst);

    for (int index = 0; index < static_cast<int>(m_childItems.size()); index++)
    {
        auto & item = m_childItems[static_cast<uint>(index)];
        if(item->m_state != StateType::StateType_valid)
        {
            auto pmodel = const_cast<FileSelectorStandardModel*>(reinterpret_cast<const FileSelectorStandardModel*>(parent.model()));
            emit pmodel->rowDataChanged(parent);
            return;
        }
    }
}

FileSelectorModelItem::StateType FileSelectorModelItem::getState()
{
    return m_state;
}

void FileSelectorModelItem::setState(FileSelectorModelItem::StateType state)
{
    m_state = state;
}
