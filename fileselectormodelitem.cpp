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

#if (defined(_WIN32) || defined(_WIN64)) && !defined(S_ISDIR)
#   define S_ISDIR(X) (((X) & _S_IFDIR) == _S_IFDIR)
#endif

#include <QCollator>
#include <QDateTime>
#include <QDir>

#include <thread>
#include <algorithm>

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

#if 1
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

FileSelectorModelItem::FileSelectorModelItem(FileSelectorModelItem *parentItem, FileSelectorStandardModel *fileSelectorStandardModel, eLocations m_locationId)
    : m_parentItem(parentItem)
    , m_fM(fileSelectorStandardModel)
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
    auto itemPtr = new FileSelectorModelItem(this, m_fM);
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

    auto itemPtr = new FileSelectorModelItem(this, m_fM);
    itemPtr->m_itemName = name;
    itemPtr->m_Path = path;
    itemPtr->m_isDir = S_ISDIR(flstat.st_mode);

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

FileSelectorModelItem *FileSelectorModelItem::child(int row)
{
    if (row < 0 || row >= static_cast<int32_t>(m_childItems.size()))
        return nullptr;
    return m_childItems[static_cast<uint32_t>(row)];
}

int FileSelectorModelItem::fetchChild(bool reload)
{
    if (m_locationId == eLocations::eLocations_folder_recent)
    {
        qDeleteAll(m_childItems);
        m_childItems.clear();

        std::vector<std::pair<eLocations, std::vector<QString>>> locationsList;
        FileSelectorLocations::GetRecentLocations(locationsList);

        if(locationsList.size() > 0)
        {
            const auto & xRecent = locationsList.front().second;
            for (decltype(xRecent.size()) recentIndex = 0; recentIndex < xRecent.size(); recentIndex++) {

                auto itemPtr = new FileSelectorModelItem(this, m_fM);

                itemPtr->m_itemName = xRecent[recentIndex];
                auto newTmpPath = xRecent[recentIndex];
                itemPtr->m_Path = QDir::toNativeSeparators(QDir(newTmpPath).absolutePath());

                struct stat64 flstat{};
                auto retcode = stat64(itemPtr->m_Path.toStdString().c_str(), &flstat);
                if (retcode != 0) // error
                    continue;

                if S_ISREG(flstat.st_mode){
                    itemPtr->m_fileSizeBytes = flstat.st_size;
                    itemPtr->m_isDir = false;
                }else{
                    continue;
                }

                itemPtr->m_filetime = flstat.st_mtime;

                m_childItems.emplace_back(itemPtr);
            }
        }
        return static_cast<int32_t>(m_childItems.size());
    }

    if (!reload && (m_fM->IsRepaint() || m_locationId != eLocations::eLocations_None))
        return static_cast<int32_t>(m_childItems.size());


    if (!m_isDir)
        return 0;

    const auto basePath = getPath();

    if (basePath.isEmpty())
        return static_cast<int32_t>(m_childItems.size());

    if (m_childItems.size() > 500)
        return static_cast<int32_t>(m_childItems.size());

#ifdef SPEEDTEST
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif // SPEEDTEST

    int32_t newItem = 0;

#if defined(USESTDFILESYSTEM)

#error android fuck-up

#if defined(_WIN32) || defined(_WIN64)
    const auto path = basePath.toStdWString();
#else
    const auto path = basePath.toStdString();
#endif

    try
    {
        if (!std::filesystem::exists(path))
        {
            if (m_childItems.size())
            {
                qDeleteAll(m_childItems);
                m_childItems.clear();
            }
            return 0;
        }
    }
    catch (const std::exception&)
    {
        return 0;
    }

    try
    {

        for (const auto & fd : std::filesystem::directory_iterator(path))
        {

#if defined(_WIN32) || defined(_WIN64)
            const auto tmpId = fd.path().wstring();
#else
            const auto tmpId = fd.path().string();
#endif
            if (m_childItemsMap.find(tmpId) == m_childItemsMap.end())
            {
                m_childItemsMap[tmpId] = true;

                auto itemPtr = new FileSelectorModelItem(this, m_fM);
                itemPtr->m_itemName = QString::fromWCharArray(fd.path().filename().wstring().c_str());

                if (!fd.is_directory())
                {
                    itemPtr->m_fileSizeBytes = static_cast<int64_t>(fd.file_size());
                    itemPtr->m_isDir = false;
                }
                itemPtr->m_filetime = static_cast<int64_t>(fd.last_write_time().time_since_epoch().count());
#if defined(_WIN32) || defined(_WIN64)
                itemPtr->m_Path = QString::fromWCharArray(tmpId.c_str());
#else
                itemPtr->m_Path = QString::fromUtf8(tmpId.c_str());
#endif
                m_childItems.emplace_back(itemPtr);
                newItem++;
            }
        }
    }
    catch (const std::exception&)
    {
        return 0;
    }

#elif defined(_WIN32) || defined(_WIN64)

    const auto path = (basePath.back() == QLatin1Char('\\') || basePath.back() == QLatin1Char('/')) ?
                (basePath + QLatin1String("*")).toStdWString() : (basePath + QLatin1String("\\*")).toStdWString();

    WIN32_FIND_DATAW fd;
    auto hFind = ::FindFirstFileW(path.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    do
    {

        if ((wcsncmp(L".", fd.cFileName, 1) == 0) || (wcsncmp(L"..", fd.cFileName, 2) == 0))
            continue;

        const std::wstring tmpId(fd.cFileName);

        if (m_childItemsMap.find(tmpId) == m_childItemsMap.end())
        {
            m_childItemsMap[tmpId] = true;

            auto itemPtr = new FileSelectorModelItem(this, m_fM);

            itemPtr->m_itemName = QString::fromWCharArray(fd.cFileName);

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
            {

            }
            else
            {
                LARGE_INTEGER filesize;
                filesize.LowPart = fd.nFileSizeLow;
                filesize.HighPart = static_cast<LONG>(fd.nFileSizeHigh);
                itemPtr->m_fileSizeBytes = filesize.QuadPart;
                itemPtr->m_isDir = false;
            }
            itemPtr->m_filetime = (static_cast<int64_t>(fd.ftLastWriteTime.dwHighDateTime) << 32) + fd.ftLastWriteTime.dwLowDateTime;
            itemPtr->m_Path = QDir::toNativeSeparators(basePath + QLatin1Char('/') + itemPtr->m_itemName);

            m_childItems.emplace_back(itemPtr);
            newItem++;

        }

    } while (::FindNextFileW(hFind, &fd) != 0);

    ::FindClose(hFind);

#else // UNIX

    DIR *dir = ::opendir(basePath.toStdString().c_str());
    if (dir == nullptr)
        return 0;

    struct dirent *dirEntry;

    while ((dirEntry = ::readdir(dir)) != nullptr)
    {
        if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)
            continue;

        std::string tmpId(dirEntry->d_name);
        if (m_childItemsMap.find(tmpId) == m_childItemsMap.end())
        {
            m_childItemsMap[tmpId] = true;

            auto itemPtr = new FileSelectorModelItem(this, m_fM);
            itemPtr->m_itemName = QString::fromUtf8(dirEntry->d_name);
            auto newTmpPath = basePath + QLatin1Char('/') + itemPtr->m_itemName;
            itemPtr->m_Path = QDir::toNativeSeparators(QDir(newTmpPath).absolutePath());

            struct stat64 flstat{};
            auto retcode = stat64(itemPtr->m_Path.toStdString().c_str(), &flstat);
            if (retcode != 0) // error
                continue;

            if (S_ISDIR(flstat.st_mode))
            {
            }
            else if S_ISREG(flstat.st_mode)
            {
                itemPtr->m_fileSizeBytes = flstat.st_size;
                itemPtr->m_isDir = false;
            }
            else
            {
                continue;
            }

            itemPtr->m_filetime = flstat.st_mtime;

            m_childItems.emplace_back(itemPtr);
            newItem++;
        }
    }

    closedir(dir);


#endif // UNIX

    if (newItem > 0)
        SortChild(SortType::SortType_name_dirFirst);

#ifdef SPEEDTEST
    if (newItem)
    {
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        auto debugInfo = QString("C T:%1 N:%2 S:%3 %4")
                .arg(static_cast<int32_t>(m_childItems.size()))
                .arg(newItem)
                .arg(diff.count(), 0, 'f', 6)
                .arg(m_Path);
        qDebug() << debugInfo;
    }
#endif // SPEEDTEST

    return static_cast<int32_t>(m_childItems.size());
}

int FileSelectorModelItem::childCount(bool firstChildOnly)
{

    if (!m_isDir)
        return 0;

    if (m_fM->IsRepaint() || m_locationId != eLocations::eLocations_None)
        return static_cast<int32_t>(m_childItems.size());

    const auto basePath = getPath();

    if (/*m_childItems.size() > 1 ||*/ basePath.isEmpty())
        return static_cast<int32_t>(m_childItems.size());

    int itemCount = 0;

#if defined(_WIN32) || defined(_WIN64)
    const auto path = (basePath.back() == QLatin1Char('\\') || basePath.back() == QLatin1Char('/')) ?
                (basePath + QLatin1String("*")).toStdWString() : (basePath + QLatin1String("\\*")).toStdWString();

    WIN32_FIND_DATAW fd;
    auto hFind = ::FindFirstFileW(path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            if ((wcsncmp(L".", fd.cFileName, 1) == 0) || (wcsncmp(L"..", fd.cFileName, 2) == 0))
                continue;
            itemCount++;
            if (firstChildOnly)
                break;
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
            itemCount++;
            if (firstChildOnly)
                break;
        }
        closedir(dir);
    }
#endif // UNIX
    return itemCount;
}

int FileSelectorModelItem::row() const
{
    if (!m_parentItem)
        return 0;
#if 0
    auto childPtr = m_parentItem->m_childItems; //std::vector<FileSelectorModelItem*> m_childItems;
    auto itr = std::find(childPtr.begin(), childPtr.end(), this);
    if (itr != childPtr.cend())
        return static_cast<int32_t>(std::distance(childPtr.begin(), itr));
#else
    auto ptr = m_parentItem->m_childItems.data(); //std::vector<FileSelectorModelItem*> m_childItems;
    const auto elementCount = static_cast<int32_t>(m_parentItem->m_childItems.size());
    for (int ind = 0; ind < elementCount; ind++)
    {
        if (*ptr++ == this)
            return ind;
    }
#endif
    return 0;
}

bool FileSelectorModelItem::hasChildren(const QModelIndex &)
{
    if (m_locationId != eLocations::eLocations_None)
        return true;

    const auto hasChild = fetchChild(false) > 0;
    return hasChild;
}

void FileSelectorModelItem::SortChild(FileSelectorModelItem::SortType sortType)
{
    QCollator coll;
#if defined (Q_OS_ANDROID)
    coll.setNumericMode(false);
#else
    coll.setNumericMode(true);
#endif
    std::sort(m_childItems.begin(), m_childItems.end(), [&coll, &sortType](const FileSelectorModelItem *s1, const FileSelectorModelItem *s2)
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
