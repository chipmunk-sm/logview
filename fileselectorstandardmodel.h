/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTORSTANDARDMODEL_H
#define FILESELECTORSTANDARDMODEL_H

#include "iconhelper.h"

#include <QAbstractItemModel>
#include <QFont>

#if defined(QT_QUICK_LIB)
#include <QQuickImageProvider>
#include <QtQuick>

class ImgItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    Q_PROPERTY(bool isIconEnabled READ isIconEnabled WRITE setIconEnabled NOTIFY enabledIconChanged)
    Q_PROPERTY(bool isIconChecked READ isIconChecked WRITE setIconChecked NOTIFY checkedIconChanged)
    Q_PROPERTY(QIcon iconObj READ getIcon WRITE setIcon NOTIFY iconChanged)
    QML_ELEMENT

public:
    ImgItem(QQuickItem *parent = 0)
        : QQuickPaintedItem(parent)
    {
    }

    void paint(QPainter *painter)
    {
        int w = painter->device()->width();
        int h = painter->device()->height();
        if(!m_ico.isNull())
        {
            QPixmap pixmap = m_ico.pixmap(QSize(h, h),
                isIconEnabled() ? QIcon::Normal: QIcon::Disabled,
                isIconChecked() ? QIcon::On : QIcon::Off);
            QPoint pos(0, 0);
            painter->drawPixmap(pos, pixmap);
        }
        else
        {
            painter->setViewport(0, h, w, -h);
            painter->fillRect(0, 0, w, h, QColor(255, 255, 255));
            // draw some text
            painter->setPen(QColor(255, 0, 0));
            painter->setFont(QFont("Times", 8, QFont::Normal, true));
            painter->drawText(w / 8, h / 6, QDateTime::currentDateTime().toString());
            painter->setPen(QPen(QBrush(QColor(0, 0, 0)) ,10));
            painter->setBrush(QColor(0, 0, 255));
        }
    }

    bool isIconEnabled() const
    {
        return m_iconEnabled;
    }
    bool isIconChecked() const
    {
        return m_iconChecked;
    }

    QIcon getIcon() const
    {
        return m_iconObj;
    }

public slots:
    void setIconEnabled(bool val)
    {
        m_iconEnabled = val;
        emit enabledIconChanged(val);
    }
    void setIconChecked(bool val)
    {
        m_iconChecked = val;
        emit checkedIconChanged(val);
    }
    void setIcon(const QIcon &icon)
    {
        m_ico = icon;
        emit iconChanged(icon);
    }

private:
    QIcon m_ico;
    bool m_iconEnabled = true;
    bool m_iconChecked = false;

    QIcon m_iconObj;

signals:
    void enabledIconChanged(bool);
    void checkedIconChanged(bool);
    void iconChanged(QIcon);
};
#endif

class FileSelectorModelItem;
//class QFileSystemModel;

class FileSelectorStandardModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    Q_PROPERTY(int     itemHeight       READ getItemHeight       WRITE setItemHeight       NOTIFY itemHeightChanged)
    Q_PROPERTY(QFont   itemFont         READ getItemFont         WRITE setItemFont         NOTIFY itemFontChanged)
    Q_PROPERTY(QString userUserPath     READ getUserUserPath     WRITE setUserUserPath     NOTIFY userUserPathChanged)
    Q_PROPERTY(QString userUserFilename READ getUserUserFilename WRITE setUserUserFilename NOTIFY userUserFilenameChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 2,
        SizeRole  ,
        TypeRole,
        DateRole,
        UrlStringRole
    };
    Q_ENUM(Role)

    FileSelectorStandardModel(QObject *parent = nullptr);
    virtual ~FileSelectorStandardModel() override;

    void RepaintMode(bool enabled) { m_repaintMode = enabled; }
    bool IsRepaint() { return m_repaintMode; }

    Q_INVOKABLE QString dataUrl(const QModelIndex &index);
    Q_INVOKABLE QIcon dataImg(const QModelIndex &index) const;

    int getItemHeight() const { return m_itemHeight; }
    QFont getItemFont() const { return m_itemFont; }
    QString getUserUserPath() const { return m_userUserPath; }
    QString getUserUserFilename() const { return m_userUserFilename; }

public slots:
    void setItemHeight(int itemHeight);
    void setItemFont(QFont itemFont);
    void setUserUserPath(QString userUserPath);
    void setUserUserFilename(QString userUserFilename);
    void rowDataChange(const QModelIndex &parent);
    void onRowDataChange(const QModelIndex &parent);

signals:
    void itemHeightChanged(int itemHeight);
    void itemFontChanged(QFont itemFont);
    void userUserPathChanged(QString userUserPath);
    void userUserFilenameChanged(QString userUserFilename);
    void rowDataChanged(const QModelIndex &parent);

private:
    constexpr static int32_t m_columnCount = 4;
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual bool hasChildren(const QModelIndex &parent) const override;
    FileSelectorModelItem *getItem(const QModelIndex &index) const;

    FileSelectorModelItem* m_root = nullptr;

    IconHelper m_icon;
    bool m_repaintMode = false;
    int m_itemHeight = 16;
    QFont m_itemFont;
    QString m_userUserPath;
    QString m_userUserFilename;

};

#endif // FILESELECTORSTANDARDMODEL_H
