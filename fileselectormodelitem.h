/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTORMODELITEM_H
#define FILESELECTORMODELITEM_H

#include "fileselectorstandardmodel.h"

#include <QString>
#include <QModelIndex>
#include <map>
#include <condition_variable>

class FileSelectorModelItem
{
public:

    typedef enum StateType
    {
        StateType_new,
        StateType_update,
        StateType_valid,
        StateType_unknown
    }StateType;

    typedef enum SortType
    {
        SortType_name,
        SortType_name_dirFirst,
        SortType_size,
        SortType_type,
        SortType_date
    }SortType;

    explicit FileSelectorModelItem(FileSelectorModelItem *parentItem, eLocations m_locationId = eLocations::eLocations_None);
    ~FileSelectorModelItem();
    int columnCount() const;
    FileSelectorModelItem* appendChild(eLocations locationId);
    void appendChild(const QString & name, const QString & path);
    void appendChild(const QString & name, const QString & path, const bool isDir, const int64_t &fileSizeBytes, const int64_t &filetime);
    QString bytesToString(int64_t val, int64_t base = 1024) const;
    FileSelectorModelItem *child(int row) const;
    int fetchChild(const QModelIndex &parent);
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
    void testChild(const QModelIndex &parent);
    StateType getState();
    void setState(StateType state);
    std::vector<FileSelectorModelItem*> m_childItems;

private:
    FileSelectorModelItem* m_parentItem = nullptr;

    eLocations   m_locationId = eLocations::eLocations_None;
    bool         m_isDir = true;
    int64_t      m_fileSizeBytes = 0;
    int64_t      m_filetime = 0;
    StateType    m_state = StateType::StateType_unknown;
    time_t       m_lasttime = 0;

    QString m_itemName;
    QString m_Path;

};

#endif // FILESELECTORMODELITEM_H
