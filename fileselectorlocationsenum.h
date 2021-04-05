/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTORLOCATIONSENUM_H
#define FILESELECTORLOCATIONSENUM_H

#include <QString>
#include <QStandardPaths>

//QStandardPaths::FontsLocation
//QStandardPaths::ApplicationsLocation
//QStandardPaths::CacheLocation
//QStandardPaths::RuntimeLocation
//QStandardPaths::GenericCacheLocation
//QStandardPaths::GenericConfigLocation
//QStandardPaths::AppDataLocation
//QStandardPaths::AppConfigLocation

#define THEMEICON()\
    MACRODEF(None, QObject::tr(""), static_cast<QStandardPaths::StandardLocation>(-1))\
    MACRODEF(Root, QObject::tr(""), static_cast<QStandardPaths::StandardLocation>(-1))\
    \
    MACRODEF(folder_recent,     QObject::tr("Recent"),           static_cast<QStandardPaths::StandardLocation>(-2))\
    MACRODEF(folder_documents,  QObject::tr("Documents"),        QStandardPaths::DocumentsLocation)\
    MACRODEF(folder_download,   QObject::tr("Download") ,        QStandardPaths::DownloadLocation)\
    MACRODEF(folder_music,      QObject::tr("Music")    ,        QStandardPaths::MusicLocation)\
    MACRODEF(folder_videos,     QObject::tr("Videos")   ,        QStandardPaths::MoviesLocation)\
    MACRODEF(folder_pictures,   QObject::tr("Pictures") ,        QStandardPaths::PicturesLocation)\
    \
    MACRODEF(folder_temp,       QObject::tr("Temp location"),    QStandardPaths::TempLocation)\
    MACRODEF(folder_data,       QObject::tr("Data location"),    QStandardPaths::DataLocation)\
    MACRODEF(folder_config,     QObject::tr("Config location"),  QStandardPaths::ConfigLocation)\
    MACRODEF(folder_generic,    QObject::tr("Generic location"), QStandardPaths::GenericDataLocation)\
    \
    MACRODEF(folder_desktop,    QObject::tr("Desktop"),          QStandardPaths::DesktopLocation)\
    MACRODEF(folder_home,       QObject::tr("Home"),             QStandardPaths::HomeLocation)\
    \
    MACRODEF(folder_internalsd, QObject::tr("Internal memory"),  static_cast<QStandardPaths::StandardLocation>(-1))\
    MACRODEF(folder_externalsd, QObject::tr("External SD"),      static_cast<QStandardPaths::StandardLocation>(-1))\
    \
    MACRODEF(folder_drive,      QObject::tr("Storage"), static_cast<QStandardPaths::StandardLocation>(-1))\

enum class eLocations
{

    #undef  MACRODEF
    #define MACRODEF(enumId, valueName, pathId) eLocations_##enumId,

    THEMEICON()

};

class LocationsEnum
{
public:
    static const char* GetLocationIconName(eLocations locationId);
    static const QString GetLocationName(eLocations locationId);
    static QStandardPaths::StandardLocation GetLocationQtId(eLocations locationId);
private:
    template<typename T, size_t elementCount>
    static T GetLocationObj(T (&arrayObj)[elementCount], eLocations locationId);
};


#endif // FILESELECTORLOCATIONSENUM_H
