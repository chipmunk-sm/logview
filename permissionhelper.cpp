/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "permissionhelper.h"

#include <QObject>
#include <QString>

#if defined (Q_OS_ANDROID)
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#endif // defined (Q_OS_ANDROID)

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

bool PermissionHelper::checkPermission(PermissionHelper::PermissionType type)
{

#if defined (Q_OS_ANDROID)

    switch (type)
    {
        case PermissionType::Storage:
            return PermissionHelper::checkPermission(QLatin1String("android.permission.STORAGE"));
        case PermissionType::External_Storage:
            return PermissionHelper::checkPermission(QLatin1String("android.permission.READ_EXTERNAL_STORAGE"));
    }

#endif // defined (Q_OS_ANDROID)
    Q_UNUSED(type);
    return true;

}

bool PermissionHelper::checkPermission(const QString& permissionName)
{

#if defined (Q_OS_ANDROID)

    try
    {
        if (QtAndroid::androidSdkVersion() < 23 ||
                QtAndroid::checkPermission(permissionName) == QtAndroid::PermissionResult::Granted)
            return true;

        const auto &requestResult = QtAndroid::requestPermissionsSync(QStringList() << permissionName);

        QAndroidJniEnvironment env;
        if (env->ExceptionCheck())
            env->ExceptionClear();

        if (!requestResult.contains(permissionName))
        {
            LOG_WARNING_A(QString("No permission found for key: %1").arg(qPrintable(permissionName)).toStdString().c_str());
            return false;
        }

        if (requestResult[permissionName] == QtAndroid::PermissionResult::Denied)
            return false;

        return true;

    }
    catch (...)
    {
        LOG_WARNING_A(QString("Unexpected eception: Failed on permission request [%1]").arg(qPrintable(permissionName)).toStdString().c_str());
    }

#endif // defined (Q_OS_ANDROID)
    Q_UNUSED(permissionName);
    return false;
}
