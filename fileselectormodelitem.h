/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTORMODELITEM_H
#define FILESELECTORMODELITEM_H

#include "fileselectorstandardmodel.h"

#include <QString>
#include <QModelIndex>
#include <map>

class FileSelectorModelItem
{
public:
    typedef enum SortType
    {
        SortType_name,
        SortType_name_dirFirst,
        SortType_size,
        SortType_type,
        SortType_date
    }SortType;

    explicit FileSelectorModelItem(FileSelectorModelItem *parentItem, FileSelectorStandardModel* fileSelectorStandardModel,
        eLocations m_locationId = eLocations::eLocations_None);
    ~FileSelectorModelItem();
    int columnCount() const;
    FileSelectorModelItem* appendChild(eLocations locationId);
    void appendChild(const QString & name, const QString & path);
    QString bytesToString(int64_t val, int64_t base = 1024) const;
    FileSelectorModelItem *child(int row);
    int fetchChild();
    int childCount(bool firstChildOnly);
    int row() const;
    bool hasChildren(const QModelIndex &parent);
    void SortChild(SortType sortType);
    QString getItemName() const;
    QString getItemSize() const;
    int64_t getItemSizeBytes() const;
    QString getItemType() const;
    QString getItemDate() const;
    QString getPath() const;
    FileSelectorModelItem *getParentItem() const;
    bool isDir() const;
    int64_t getFileSizeBytes() const;
    int64_t getFiletime() const;
    QString getItemExtension() const;
    eLocations getLocationId() const;

private:
    FileSelectorModelItem *m_parentItem = nullptr;
    FileSelectorStandardModel* m_fM = nullptr;
    std::vector<FileSelectorModelItem*> m_childItems;

#if defined(_WIN32) || defined(_WIN64)
    std::map<std::wstring, bool> m_childItemsMap;
#else
    std::map<std::string, bool> m_childItemsMap;
#endif

    eLocations   m_locationId = eLocations::eLocations_None;
    bool         m_isDir = true;
    int64_t      m_fileSizeBytes = 0;
    int64_t      m_filetime = 0;

    QString m_itemName;
    QString m_Path;

};

#endif // FILESELECTORMODELITEM_H
