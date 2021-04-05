/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QPixmap>
#include <QIcon>
#include "fileselectorlocationsenum.h"

class QMimeDatabase;

class IconHelper
{
public:
    explicit IconHelper();
    ~IconHelper();

    constexpr static double m_dafault_scale = 2.5;
    constexpr static double m_dafault_scale_min = 1.5;
    constexpr static double m_dafault_scale_max = 5.0;

    static QPixmap CreateIcon(const QString &iconTextT, const QString& iconTextB, QFont newFont);
    static QString GetIconSize(const QFont & fontX, double change = 0.0);
    static QString GetIconSizeInPersent();
    static void ResetIconSize();
    static QString GetIconScale();
    QIcon GetNativeIcon(const QString & filename) const;
    QIcon GetIconForFile(const QString &filename) const;
    QIcon GetDirIcon() const;
    QIcon GetTemeIcon(eLocations themeIcon) const;

private:
    QMimeDatabase *m_mime_database = nullptr;
    mutable std::map<QString, QIcon> m_iconCache;
    QIcon m_icoDir;
    QIcon m_icoFile;
};

#endif // ICONHELPER_H
