/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselectorlocations.h"
#include "fileselectorlocationsenum.h"
#include "permissionhelper.h"

#include <QObject>
#include <QCoreApplication>
#include <QStorageInfo>
#include <QSettings>

#ifdef Q_OS_ANDROID
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#endif // Q_OS_ANDROID

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

void FileSelectorLocations::getRootPathList(std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList, bool skipAppFolder)
{

    PermissionHelper::checkPermission(PermissionHelper::PermissionType::Storage);
    PermissionHelper::checkPermission(PermissionHelper::PermissionType::External_Storage);

    //GetRecentLocations(locationsList);
    std::vector<QString> tmp;
    locationsList.emplace_back(eLocations::eLocations_folder_recent, tmp);

    GetLocations(eLocations::eLocations_folder_documents, locationsList, skipAppFolder);
    GetLocations(eLocations::eLocations_folder_download,  locationsList, skipAppFolder);
    GetLocations(eLocations::eLocations_folder_music,     locationsList, skipAppFolder);
    GetLocations(eLocations::eLocations_folder_videos,    locationsList, skipAppFolder);
    GetLocations(eLocations::eLocations_folder_pictures,  locationsList, skipAppFolder);

    if(!skipAppFolder)
    {
        GetLocations(eLocations::eLocations_folder_temp,    locationsList, skipAppFolder);
#if (QT_VERSION_MAJOR < 6)
        GetLocations(eLocations::eLocations_folder_data,    locationsList, skipAppFolder);
#endif
        GetLocations(eLocations::eLocations_folder_config,  locationsList, skipAppFolder);
        GetLocations(eLocations::eLocations_folder_generic, locationsList, skipAppFolder);
    }

    GetLocations(eLocations::eLocations_folder_desktop, locationsList, skipAppFolder);
    GetLocations(eLocations::eLocations_folder_home,    locationsList, skipAppFolder);

#ifdef Q_OS_ANDROID
    GetAndroidLocations(eLocations::eLocations_folder_internalsd, locationsList);
    GetAndroidLocations(eLocations::eLocations_folder_externalsd, locationsList);
#endif // Q_OS_ANDROID

    GetRootLocations(eLocations::eLocations_folder_drive, locationsList);

}

void FileSelectorLocations::GetLocations(eLocations locationId, std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList, bool skipAppFolder)
{
    std::vector<QString> tmp;
    for (auto &itemLoc : QStandardPaths::standardLocations(LocationsEnum::GetLocationQtId(locationId)))
    {
        if(itemLoc.isEmpty() || (skipAppFolder && itemLoc.contains(QCoreApplication::applicationName(), Qt::CaseInsensitive)))
            continue;
        try
        {
            if(QDir(itemLoc).exists())
                tmp.emplace_back(itemLoc);
        }
        catch (...)
        {
            LOG_ERROR_A("GetLocation: Unexpected exception");
        }
    }
    if(!tmp.empty())
        locationsList.emplace_back(locationId, tmp);
}

void FileSelectorLocations::GetAndroidLocations(eLocations locationId, std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList)
{
    Q_UNUSED(locationId);
    Q_UNUSED(locationsList);

#ifdef Q_OS_ANDROID
    std::vector<QString> tmp;

    try
    {
        QAndroidJniObject mediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment",
           locationId == eLocations::eLocations_folder_externalsd ? "getExternalStorageDirectory" : "getDataDirectory",
           "()Ljava/io/File;");

        QAndroidJniObject mediaPath = mediaDir.callObjectMethod("getAbsolutePath", "()Ljava/lang/String;");
        QString dataAbsPath = mediaPath.toString();
        QAndroidJniEnvironment env;
        if (env->ExceptionCheck())
            env->ExceptionClear();
        else
            tmp.emplace_back(dataAbsPath);
    }
    catch (...)
    {
        LOG_WARNING_A("GetAndroidLocation: Unexpected exception");
        try// c++xx >= c++11
        {
            std::exception_ptr curr_excp = std::current_exception();
            if (curr_excp)
                std::rethrow_exception(curr_excp);
        }
        catch (const std::exception& e)
        {
            LOG_WARNING_A(e.what());
        }
    }

    if(tmp.size())
        locationsList.emplace_back(locationId, tmp);

#endif // Q_OS_ANDROID
}

void FileSelectorLocations::GetRootLocations(eLocations locationId, std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList)
{
    std::vector<QString> tmp;
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes())
    {
        if (!storage.isValid() || !storage.isReady() || storage.isReadOnly())
            continue;

        const auto path = storage.rootPath();
        if (path.startsWith("/sys") || path.startsWith("/run") || path.startsWith("/dev") || path.startsWith("/proc"))
            continue;

        tmp.emplace_back(path);

    }
    locationsList.emplace_back(locationId, tmp);
}

void FileSelectorLocations::GetRecentLocations(std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList)
{
    auto resList = AddRecentLocations(QString(), false);

    std::vector<QString> tmp;
    foreach (const auto &item, resList)
        tmp.emplace_back(item);

    locationsList.emplace_back(eLocations::eLocations_folder_recent, tmp);
}

void FileSelectorLocations::ClearRecentLocations()
{
    FileSelectorLocations::AddRecentLocations(QString(), true);
}

QStringList FileSelectorLocations::AddRecentLocations(const QString &path, bool clear)
{
    constexpr auto mruKeyName = "files/MRUlist";

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    settings.sync();

    if(clear)
    {
        settings.setValue(mruKeyName, QStringList());
        settings.sync();
    }

    auto resList = settings.value(mruKeyName).toStringList();
    if(path.isEmpty())
        return resList;

    auto pos = resList.indexOf(path);
    if(pos != -1)
        resList.removeAt(pos);

    resList.push_front(path);

    if (resList.size() > 10)
        resList.pop_back();

    settings.setValue(mruKeyName, resList);

    settings.sync();

    return resList;
}
