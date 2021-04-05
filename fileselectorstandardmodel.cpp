/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselectorlocations.h"
#include "fileselectormodelitem.h"
#include "fileselectorstandardmodel.h"

#include <QDir>
#include <QTreeView>
#include <QTime>
#include <QHeaderView>
#include <thread>

#ifndef LOG_ERROR_A
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define LOG_ERROR_A(mess) qFatal("%s", mess)
#   define LOG_WARNING_A(mess) qWarning("%s", mess)
#endif // LOG_ERROR_A


FileSelectorStandardModel::FileSelectorStandardModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new FileSelectorModelItem(nullptr, this, eLocations::eLocations_Root))
{

    std::vector<std::pair<eLocations, std::vector<QString>>> locationsList;
    FileSelectorLocations::getRootPathList(locationsList, false);

    for (auto & number : locationsList)
    {
        auto itemPtr = m_root->appendChild(number.first);
        for (const auto &subitem : number.second)
        {
            auto nativePath = QDir::toNativeSeparators(subitem);
            itemPtr->appendChild(nativePath, nativePath);
        }
    }
    auto threadBase = [](FileSelectorStandardModel* pThis)
    {
        try
        {
            for (int ind = 0; ind < pThis->m_root->childCount(false); ind++) {
                auto tmpRoot = pThis->m_root->child(ind);
                if(!tmpRoot)
                    continue;

                tmpRoot->fetchChild();
                //qDebug() << "Root " << tmpRoot->getItemName();

                for (int L1 = 0; L1 < tmpRoot->childCount(false); L1++) {

                    auto tmpL1 = tmpRoot->child(L1);
                    if(!tmpL1)
                        continue;

                    tmpL1->fetchChild();
                    //qDebug() << "  L1 " << tmpL1->getItemName();

                    if (!tmpL1->isDir()) {
                        pThis->m_icon.GetIconForFile(tmpL1->getPath());
                        continue;
                    }
                    auto countL2 = std::min<int>(100, tmpL1->childCount(false));
                    for (int L2 = 0; L2 < countL2; L2++) {
                        auto tmpL2 = tmpL1->child(L2);
                        if(!tmpL2)
                            continue;
                        //qDebug() << "    L2 " << tmpL2->getItemName();

                        if (!tmpL2->isDir())
                            pThis->m_icon.GetIconForFile(tmpL2->getPath());
                    }
                }
            }
        }
        catch(...)
        {

        }
    };
    (std::thread(threadBase, this)).detach();
}

FileSelectorStandardModel::~FileSelectorStandardModel()
{
    delete m_root;
}

QString FileSelectorStandardModel::dataUrl(const QModelIndex &index)
{
    return getItem(index)->getPath();
}

QIcon FileSelectorStandardModel::dataImg(const QModelIndex &index)
{
    if (index.column() != 0)
        return QIcon();

    auto item = getItem(index);

    auto locationId = item->getLocationId();
    if (locationId != eLocations::eLocations_None)
        return m_icon.GetTemeIcon(locationId);

    if (item->isDir())
        return m_icon.GetDirIcon();

    return m_icon.GetIconForFile(item->getPath());
}

void FileSelectorStandardModel::setItemHeight(int itemHeight)
{
    if (m_itemHeight == itemHeight)
        return;
    m_itemHeight = itemHeight;
    emit itemHeightChanged(m_itemHeight);
}

void FileSelectorStandardModel::setItemFont(QFont itemFont)
{
    if (m_itemFont == itemFont)
        return;
    m_itemFont = itemFont;
    QFontMetrics fm(itemFont);

    const auto maxSize = fm.height() + fm.lineSpacing();
    setItemHeight(maxSize);
    emit itemFontChanged(m_itemFont);
}

void FileSelectorStandardModel::setUserUserPath(QString userUserPath)
{
    if (m_userUserPath == userUserPath)
        return;
    m_userUserPath = userUserPath;
    emit userUserPathChanged(m_userUserPath);
}

void FileSelectorStandardModel::setUserUserFilename(QString userUserFilename)
{
    if (m_userUserFilename == userUserFilename)
        return;
    m_userUserFilename = userUserFilename;
    emit userUserFilenameChanged(m_userUserFilename);
}

QHash<int, QByteArray> FileSelectorStandardModel::roleNames() const
{
    static const QHash<int, QByteArray> roles
        {
            { NameRole, "nameRole"},
            { SizeRole, "sizeRole"},
            { TypeRole, "typeRole"},
            { DateRole, "dateRole"},
            { UrlStringRole, "urlStringRole"},
            };
    return roles;
}

QVariant FileSelectorStandardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //DEBUGTRACE();
    if (Qt::DisplayRole == Qt::ItemDataRole(role))
    {
        switch (section)
        {
        case 0: return QObject::tr("Name");
        case 1: return QObject::tr("Size");
        case 2: return QObject::tr("Type");
        case 3: return QObject::tr("Date modified");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags FileSelectorStandardModel::flags(const QModelIndex &index) const
{
    //DEBUGTRACE();
    return QAbstractItemModel::flags(index);
}

int FileSelectorStandardModel::columnCount(const QModelIndex &) const
{
    return m_columnCount;
}

int FileSelectorStandardModel::rowCount(const QModelIndex &parent = QModelIndex()) const
{
    if (parent.column() > 0)
        return 0;
    return getItem(parent)->fetchChild();
}

QVariant FileSelectorStandardModel::data(const QModelIndex &index, int role) const
{
    //DEBUGTRACE();
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0: return getItem(index)->getItemName();
        case 1: return getItem(index)->getItemSize();
        case 2: return getItem(index)->getItemType();
        case 3: return getItem(index)->getItemDate();
        }
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
        case 0: return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case 1: return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        case 2: return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        case 3: return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        return QVariant();
    }

    if (role == Qt::DecorationRole)
    {
        if (index.column() != 0)
            return QVariant();

        auto item = getItem(index);
        auto locationId = item->getLocationId();

        if (locationId != eLocations::eLocations_None)
            return m_icon.GetTemeIcon(locationId);

        if (item->isDir())
            return m_icon.GetDirIcon();

        return m_icon.GetIconForFile(item->getPath());
    }

    if (role == UrlStringRole)
    {
        if (index.column() == 0)
            return getItem(index)->getPath();
        return QVariant();
    }

    switch (role)
    {
    case NameRole: return getItem(index)->getItemName();
    case SizeRole: return getItem(index)->getItemSize();
    case TypeRole: return getItem(index)->getItemType();
    case DateRole: return getItem(index)->getItemDate();
    }

    return QVariant();
}

QModelIndex FileSelectorStandardModel::index(int row, int column, const QModelIndex &parent = QModelIndex()) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto item = getItem(parent)->child(row);
    if (item)
        return createIndex(row, column, item);

    return QModelIndex();
}

QModelIndex FileSelectorStandardModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto *parentItem = getItem(index)->getParentItem();
    if (parentItem == m_root)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool FileSelectorStandardModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return false;
    return getItem(parent)->hasChildren(parent);
}

FileSelectorModelItem *FileSelectorStandardModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        auto *item = static_cast<FileSelectorModelItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return m_root;
}
