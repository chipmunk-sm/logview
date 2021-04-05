/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "logviewselectionmodel.h"

#include <QScrollBar>

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

LogViewSelectionModel::LogViewSelectionModel(QAbstractItemModel *model, QScrollBar *pSlider, QObject *parent)
    : QItemSelectionModel(model, parent)
    , m_pSlider(pSlider)
{
    DEBUGTRACE();
}

LogViewSelectionModel::~LogViewSelectionModel()
{
    DEBUGTRACE();
}

bool LogViewSelectionModel::IsSelectedEx(int32_t col, int32_t row) const
{
    //DEBUGTRACE();
    if (!m_pSlider || m_offset < 0)
        return false;

    auto currentRow = row + m_pSlider->value();

    auto bCol = m_selectFirst.column < m_selectSecond.column ?
        m_selectFirst.column <= col && col <= m_selectSecond.column :
        m_selectFirst.column >= col && col >= m_selectSecond.column;

    auto bRow = m_selectFirst.row < m_selectSecond.row ?
        m_selectFirst.row <= currentRow && currentRow <= m_selectSecond.row :
        m_selectFirst.row >= currentRow && currentRow >= m_selectSecond.row;

    return bCol & bRow;
}

bool LogViewSelectionModel::GetSelectedEx(LogViewSelectionModelItem *pItemFirst, LogViewSelectionModelItem *pItemSecond) const
{
    DEBUGTRACE();
    if (!m_pSlider || m_offset < 0)
        return false;

    if (pItemFirst)
        memcpy(pItemFirst, &m_selectFirst, sizeof(LogViewSelectionModelItem));

    if (pItemSecond)
        memcpy(pItemSecond, &m_selectSecond, sizeof(LogViewSelectionModelItem));

    return true;
}

void LogViewSelectionModel::scrollSelection(bool shiftOn)
{
    DEBUGTRACE();

    m_offset = m_pSlider->value();

    auto rowX = m_offset;
    auto ind = currentIndex();
    if (ind.isValid())
        rowX += ind.row();

    if(!shiftOn)
        m_selectFirst.row = rowX;
    m_selectSecond.row = rowX;
}

void LogViewSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
    DEBUGTRACE();

    Q_UNUSED(selection);

    if(m_updateLock)
        return;

    if (m_offset < 0)
        command = SelectionFlag::Clear;

    auto slider = m_pSlider->value();

    if ((command & SelectionFlag::Current) == SelectionFlag::Current)
    {
        m_selectSecond.column = currentIndex().column();
        m_selectSecond.row = currentIndex().row() + slider;
    }
    else if ((command & SelectionFlag::Clear) == SelectionFlag::Clear)
    {
        m_offset = slider;
        m_selectFirst.column = m_selectSecond.column = currentIndex().column();
        m_selectFirst.row = m_selectSecond.row = currentIndex().row() + m_offset;
    }
    else
    {
        m_offset = -1;
    }

    emit selectionChangedEx();

}

void LogViewSelectionModel::clear()
{
    DEBUGTRACE();
    m_offset = -1;
    QItemSelectionModel::clear();
}

void LogViewSelectionModel::reset()
{
    //DEBUGTRACE();
}

void LogViewSelectionModel::clearCurrentIndex()
{
    DEBUGTRACE();
    m_offset = -1;
    QItemSelectionModel::clearCurrentIndex();
}

