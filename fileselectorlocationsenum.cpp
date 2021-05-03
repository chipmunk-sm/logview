/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselectorlocationsenum.h"
#include <QObject>

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

static const char* LOCATION_ICONNAME[] = {

    #undef  MACRODEF
    #define MACRODEF(enumId, valueName, pathId) #enumId,

    THEMEICON()

};

static const QString LOCATION_NAME[] = {

    #undef  MACRODEF
    #define MACRODEF(enumId, valueName, pathId) valueName,

    THEMEICON()

};

static const QStandardPaths::StandardLocation LOCATION_QTID[] = {

    #undef  MACRODEF
    #define MACRODEF(enumId, valueName, pathId) pathId,

    THEMEICON()

};

template<typename T, size_t elementCount>
T LocationsEnum::GetLocationObj(T (&arrayObj)[elementCount], eLocations locationId)
{
    const auto index = static_cast<int32_t>(locationId);

    if(index < 0 || index >= static_cast<int32_t>(elementCount))
        return arrayObj[0];

    return arrayObj[index];
}

const char* LocationsEnum::GetLocationIconName(eLocations locationId)
{
    return GetLocationObj(LOCATION_ICONNAME, locationId);
}

const QString LocationsEnum::GetLocationName(eLocations locationId)
{
    return GetLocationObj(LOCATION_NAME, locationId);
}

QStandardPaths::StandardLocation LocationsEnum::GetLocationQtId(eLocations locationId)
{
    return GetLocationObj(LOCATION_QTID, locationId);
}
