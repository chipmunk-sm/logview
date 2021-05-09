/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselectorlocations.h"
#include "fileselectormodelitem.h"
#include "fileselectorstandardmodel.h"

#include <QDir>
#include <QTreeView>
#include <QTime>
#include <QHeaderView>
#include <thread>


FileSelectorStandardModel::FileSelectorStandardModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new FileSelectorModelItem(nullptr, eLocations::eLocations_Root))
{

    std::vector<std::pair<eLocations, std::vector<QString>>> locationsList;
    FileSelectorLocations::getRootPathList(locationsList, false);

    connect(this, &FileSelectorStandardModel::rowDataChanged, this, &FileSelectorStandardModel::rowDataChange);

    for (auto & number : locationsList)
    {
        auto itemPtr = m_root->appendChild(number.first);
        for (const auto &subitem : number.second)
        {
            auto nativePath = QDir::toNativeSeparators(subitem);
            itemPtr->appendChild(nativePath, nativePath);
        }
    }

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

void FileSelectorStandardModel::onRowDataChange(const QModelIndex &parent)
{
    auto items = getItem(parent);
     if(!items)
         return;

     emit layoutAboutToBeChanged();

     for (int index = 0; index < static_cast<int>(items->m_childItems.size()); index++)
     {
         if(items->m_childItems[static_cast<uint>(index)]->getState() == FileSelectorModelItem::StateType::StateType_unknown)
         {
             beginRemoveRows(parent, index, index + 1);
             delete items->m_childItems[static_cast<uint>(index)];
             items->m_childItems[static_cast<uint>(index)] = nullptr;
             items->m_childItems.erase(items->m_childItems.begin() + index);
             endRemoveRows();
             index--;
             continue;
         }
     }

     emit layoutChanged();
}

void FileSelectorStandardModel::rowDataChange(const QModelIndex &parent)
{
    QMetaObject::invokeMethod(this, "onRowDataChange", Qt::QueuedConnection, Q_ARG(QModelIndex, parent));
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
    if (!index.isValid())
        return Qt::NoItemFlags;

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

    auto item = getItem(parent);
    if(!item)
        return 0;

    auto itemCount = item->fetchChild(parent);
    return itemCount;
}

QVariant FileSelectorStandardModel::data(const QModelIndex &index, int role) const
{
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
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

//    if (!hasIndex(row, column, parent))
//        return QModelIndex();

    auto item = getItem(parent)->child(row);
    if (item)
        return createIndex(row, column, item);

    return QModelIndex();
}

QModelIndex FileSelectorStandardModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    auto *parentItem = getItem(child)->getParentItem();
    if (parentItem == m_root)
        return QModelIndex();

    auto parentItem1 = parentItem->getParentItem();
    if (parentItem1 == nullptr)
        return QModelIndex();

    auto rowIndex = 0;
    auto ptr = parentItem1->m_childItems.data();
    const auto elementCount = static_cast<int32_t>(parentItem1->m_childItems.size());
    for (int ind = 0; ind < elementCount; ind++){
        if (*ptr++ == parentItem){
            rowIndex = ind;
            break;
        }
    }
    return createIndex(rowIndex, 0, parentItem);
}

bool FileSelectorStandardModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return false;

    auto item = getItem(parent);
    if (!item)
        return false;

    auto itemCount = item->fetchChild(parent);
    return itemCount > 0;
}

FileSelectorModelItem *FileSelectorStandardModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        auto *item = static_cast<FileSelectorModelItem*>(index.internalPointer());
        if (item)
            return item;
        return nullptr;
    }
    return m_root;
}
