/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "iconhelper.h"

#include <QMimeDatabase>
#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QSettings>
#include <QDir>


#if defined(_WIN32) || defined(_WIN64)
#   define NOMINMAX
#   include <Windows.h>
#   include <shellapi.h>
#   if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PC_APP)
#   endif
#   if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#   endif
#   if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#   endif
#   if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   include <QtWin>
#   endif
#else
#   include <dirent.h>
#endif // defined(_WIN32) || defined(_WIN64)

#   include <sys/stat.h>
#   include <sys/types.h>

#if (defined(_WIN32) || defined(_WIN64)) && !defined(S_ISDIR)
#   define S_ISDIR(X) (((X) & _S_IFDIR) == _S_IFDIR)
#endif

#ifndef LOG_ERROR_A
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define LOG_ERROR_A(mess) qFatal("%s", mess)
#   define LOG_INFO_A(mess) qInfo("%s", mess)
#else
#include "clogger/clogger.h"
#endif // LOG_ERROR_A

#if 0
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

#ifndef CONFIG_SCALE_ICONS
#define CONFIG_SCALE_ICONS      "cfg/SCALE_ICONS"
#endif // CONFIG_SCALE_ICONS

IconHelper::IconHelper()
{
    m_mime_database = new QMimeDatabase();
    //m_icoFile = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    //m_icoDir = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    m_icoFile = QPixmap(":/data/file.1.svg");
    m_icoDir = QPixmap(":/data/folder.1.svg");
}

IconHelper::~IconHelper()
{
    delete m_mime_database;
}

QPixmap IconHelper::CreateIcon(const QString &iconTextT, const QString& iconTextB, QFont newFont)
{
    try
    {
        const int32_t imageSize = 512;
        const auto imageBorderSize = static_cast<int32_t>(0.1 * imageSize);

        int newPixelSize = 8;
        while (true)
        {
            newFont.setPixelSize(newPixelSize);
            QFontMetrics fm(newFont);
#if QT_VERSION_MAJOR >=5 && QT_VERSION_MINOR >= 11
            auto tmpWidth = fm.horizontalAdvance(iconTextB);
#else
            auto tmpWidth = fm.width(iconTextB);
#endif
            if (tmpWidth > imageSize - imageBorderSize)
                break;
            newPixelSize += 4;
        }

        newFont.setPixelSize(newPixelSize);

        QPixmap pix(imageSize, imageSize);
        auto pixRect = pix.rect();

        QPainter painter;
        if (!painter.begin(&pix))
            return QPixmap();

        painter.fillRect(pix.rect(), Qt::white);

        painter.setFont(newFont);
        painter.setPen(QPen(Qt::black));
        painter.drawText(pixRect, Qt::AlignHCenter | Qt::AlignTop, iconTextT);
        painter.drawText(pixRect, Qt::AlignHCenter | Qt::AlignBottom, iconTextB);

        painter.end();

        return pix;
    }
    catch (...)
    {
        LOG_INFO_A("Failed IconHelper::CreateIcon");
    }
    return QPixmap();
}

QString IconHelper::GetIconSize(const QFont & fontX, double change)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.sync();

    const auto fontSize = fontX.pixelSize() > 0 ? fontX.pixelSize() : fontX.pointSize();

    const auto iconScale = settings.value(CONFIG_SCALE_ICONS, m_dafault_scale).toDouble();

    auto newScale = iconScale + change;
    if (newScale < m_dafault_scale_min)
        newScale = m_dafault_scale_min;
    else if (newScale > m_dafault_scale_max)
        newScale = m_dafault_scale_max;

    settings.setValue(CONFIG_SCALE_ICONS, newScale);
    const auto tmp = static_cast<int>(fontSize * newScale);
    return QString::number(tmp);
}

QString IconHelper::GetIconSizeInPersent()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.sync();
    const auto iconScale = settings.value(CONFIG_SCALE_ICONS, m_dafault_scale).toDouble();
#if 1   
    auto res = iconScale * 200.0 / m_dafault_scale_max;
#else
    constexpr auto range = (m_dafault_scale_max - m_dafault_scale_min);
    auto tmpScale = (iconScale - m_dafault_scale_min);
    auto res = tmpScale * 200.0 / range;
#endif // 1
    return QString("%1").arg(res, 0, 'f', 0);
}

QString IconHelper::GetIconScale()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.sync();
    const auto iconScale = settings.value(CONFIG_SCALE_ICONS, m_dafault_scale).toDouble();
    return QString("%1").arg(iconScale, 0, 'f', 0);
}

void IconHelper::ResetIconSize()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.sync();
    settings.setValue(CONFIG_SCALE_ICONS, m_dafault_scale);
}

#if defined(_WIN32) || defined(_WIN64)
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
QIcon IconHelper::GetNativeIcon(const QString& filename) const
{
//    typedef enum sHIL
//    {
//        sHIL_EXTRALARGE = 0x2, // 48x48 or user-defined
//        sHIL_JUMBO = 0x4       // 256x256 (Vista or later)
//    }sHIL;

    const uint32_t standardSizeEntries[] =
    {
        SHGFI_SHELLICONSIZE,
        SHGFI_LARGEICON,
        SHGFI_SMALLICON,
    };

    const QString nativeName = QDir::toNativeSeparators(filename);
    const wchar_t *sourceFileC = reinterpret_cast<const wchar_t *>(nativeName.utf16());

    const uint32_t baseFlags = SHGFI_ICON | SHGFI_SYSICONINDEX | SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES;

    for (auto standardSizeEntry : standardSizeEntries)
    {
        const unsigned flags = baseFlags | standardSizeEntry;

        SHFILEINFO info;
        memset(&info, 0, sizeof(SHFILEINFO));
        const auto result = ::SHGetFileInfo(sourceFileC, 0, &info, sizeof(SHFILEINFO), flags) != 0;
        if (!result)
            continue;

        if (info.hIcon)
        {
            auto pixmap = QtWin::fromHICON(info.hIcon);
            if (!pixmap.isNull())
            {
                DestroyIcon(info.hIcon);
                return QIcon(pixmap);
            }
            DestroyIcon(info.hIcon);

//            pixmap = pixmapFromShellImageList(sHIL_EXTRALARGE, info);
//            if (!pixmap.isNull())
//            {
//                DestroyIcon(info.hIcon);
//                resultIco = QIcon(pixmap);
//                break;
//            }
//            pixmap = pixmapFromShellImageList(sHIL_JUMBO, info);
//            if (!pixmap.isNull())
//            {
//                DestroyIcon(info.hIcon);
//                resultIco = QIcon(pixmap);
//                break;
//            }
        }
    }

    return m_icoFile;
}
#else
QIcon IconHelper::GetNativeIcon(const QString& filename) const
{
   auto ret = QIcon::fromTheme(filename);
   return ret;
}
#endif
#else

#endif // defined(_WIN32) || defined(_WIN64)

QIcon IconHelper::GetIconForFile(const QString& sourceFile) const
{
    auto filename = QDir::toNativeSeparators(sourceFile);
   
#if  defined(_WIN32) || defined(_WIN64)

    struct _stat64 flstat{};
    if (_wstati64(filename.toStdWString().c_str(), &flstat) != 0)
        return m_icoFile;

#else

    struct stat64 flstat{};
    if (stat64(filename.toStdString().c_str(), &flstat) != 0)
        return m_icoFile;

#endif

    if (S_ISDIR(flstat.st_mode))
        return m_icoDir;

    QList<QMimeType> mime_types = m_mime_database->mimeTypesForFileName(filename);
    for (auto index = 0; index < mime_types.count(); index++)
    {
#if defined(_WIN32) || defined(_WIN64)
        if (sourceFile.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
            return GetNativeIcon(filename);

        auto nameGen = mime_types[index].genericIconName();
        auto res = m_iconCache.find(nameGen);
        if (res != m_iconCache.end())
           return res->second;

        auto resIcon = GetNativeIcon(filename);
        m_iconCache.insert(std::pair<QString, QIcon>(nameGen, resIcon));
        return resIcon;
#else
        auto name = mime_types[index].iconName();
        auto icon = QIcon::fromTheme(name);
        if (!icon.isNull())
            return icon;
#endif
    }
    return m_icoFile;
}

QIcon IconHelper::GetDirIcon() const
{
    return m_icoDir;
}

QIcon IconHelper::GetTemeIcon(eLocations themeIcon) const
{
    const auto name = QLatin1String(":/data/") + LocationsEnum::GetLocationIconName(themeIcon) + QLatin1String(".1.svg");
    return QPixmap(name);
}
