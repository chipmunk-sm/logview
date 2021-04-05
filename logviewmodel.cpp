/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "logviewmodel.h"

#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QApplication>
#include <QLineEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QFileDevice>
#include <QFontMetrics>




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

LogViewModel::LogViewModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_fontMetrics(QApplication::font())
{
    DEBUGTRACE();
    m_controlGeometry = QSize(128,128);
}

LogViewModel::~LogViewModel()
{
    DEBUGTRACE();
    m_dataSource.shutdown();
}

void LogViewModel::OpenFile(const QString &path)
{
    DEBUGTRACE();
    m_dataSource.open(path,[&](void)->void{ emit xrowCountChanged(nullptr);});
    emit layoutChanged();
}

int LogViewModel::columnWidth() const
{
    return m_columnWidth;
}

void LogViewModel::setColumnWidth(int columnWidth) const
{
    if (columnWidth > m_columnWidth)
    {
        m_columnWidth = columnWidth;
        emit columnWidthChanged(columnWidth);
    }
}

void LogViewModel::controlGeometryUpdate(int controlWidth, int controlHeight, int rowHeight, const QFont *newFont, int &viewRows, int &rowsInFile, int &maxScrollBar)
{
    if(controlWidth > 0)
        m_controlGeometry.setWidth(controlWidth);

    if(controlHeight > 0)
        m_controlGeometry.setHeight(controlHeight);

    if(rowHeight  > 0)
    {
        m_rowHeight = rowHeight;
        if(newFont)
            m_fontMetrics = QFontMetrics(*newFont);
    }

    m_visibleRowCount = std::max<int>(1, m_controlGeometry.height()) / std::max<int>(1, m_rowHeight);

    rowsInFile = m_dataSource.getLogLineCount();
    viewRows = std::min(rowsInFile, m_visibleRowCount);
    maxScrollBar = std::max(0, rowsInFile - viewRows);
}

void LogViewModel::setScrollbarPosition(int32_t scrollbarPosition)
{
    m_scrollbarPosition = scrollbarPosition;
}

int LogViewModel::rowCount(const QModelIndex &) const
{
    return std::min<int>(m_dataSource.getLogLineCount(), m_visibleRowCount);
}

int LogViewModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant LogViewModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.column() == 0)
    {
        auto indRow = index.row() + m_scrollbarPosition;
        auto returnString = dataEx(indRow);
        setColumnWidth(m_fontMetrics.horizontalAdvance(returnString));
        return returnString;
    }
    return QVariant();
}

QString LogViewModel::dataEx(int32_t indRow) const
{
    return m_dataSource.getLogLine(indRow);
}

QVariant LogViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //DEBUGTRACE();
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            auto validRows = std::max<int32_t>(0, m_dataSource.getLogLineCount());
            const auto position = m_scrollbarPosition;
            auto visibleRows = std::max(1, rowCount(QModelIndex()));//;
            return QString("Rows %1-%2 Total %3 ")
                    //.arg(visibleRows)
                    .arg(position + 1)
                    .arg(position +  visibleRows)
                    .arg(validRows)
                    ;
        }
        if (orientation == Qt::Vertical)
        {
            auto res = section + m_scrollbarPosition;
            return QString("%1 ").arg(res, 8, 10, QLatin1Char(' '));
        }
    }
    return QVariant();
}
