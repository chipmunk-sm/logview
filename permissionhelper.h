/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef PERMISSIONHELPER_H
#define PERMISSIONHELPER_H

class QString;

class PermissionHelper
{
public:
    enum class PermissionType
    {
        Storage,
        External_Storage
    };

    static bool checkPermission(PermissionType type);
    static bool checkPermission(const QString &permissionName);
};

#endif // PERMISSIONHELPER_H
