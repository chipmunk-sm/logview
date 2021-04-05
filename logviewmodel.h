/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef LOGVIEWMODEL_H
#define LOGVIEWMODEL_H

#include "clogdatasource.h"

#include <QAbstractTableModel>
#include <QFile>
#include <QFontMetrics>

#include <mutex>
#include <condition_variable>

class LogViewModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    explicit LogViewModel(QObject *parent);
    ~LogViewModel() override;

    void OpenFile(const QString &path);

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setScrollbarPosition(int32_t scrollbarPosition);
    int columnWidth() const;
    void controlGeometryUpdate(int controlWidth, int controlHeight, int rowHeight, const QFont *newFont, int &viewRows, int &rowsInFile, int &maxScrollBar);
    QString dataEx(int32_t indRow) const;

public slots:
    void setColumnWidth(int columnWidth) const;

signals:
    void xrowCountChanged(QFont *newFont);
    void columnWidthChanged(int colWidth) const;

private:
    int32_t                  m_scrollbarPosition = 0;
    int32_t                  m_visibleRowCount = 0;
    QFontMetrics             m_fontMetrics;
    QSize                    m_controlGeometry;
    int                      m_rowHeight = 10;
    CLogDatasource           m_dataSource;
    mutable int              m_columnWidth = 1;
};

#endif // LOGVIEWMODEL_H


