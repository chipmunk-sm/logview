/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef LOGVIEWSELECTIONMODEL_H
#define LOGVIEWSELECTIONMODEL_H

#include <QItemSelectionModel>

class QScrollBar;
class QAbstractItemModel;

class LogViewSelectionModel : public QItemSelectionModel
{
    Q_OBJECT
public:

    struct LogViewSelectionModelItem
    {
        int32_t column;
        int32_t row;
    };

    explicit LogViewSelectionModel(QAbstractItemModel *model, QScrollBar *pSlider, QObject *parent);
    virtual ~LogViewSelectionModel() override;
    bool IsSelectedEx(int32_t col, int32_t row) const;
    bool GetSelectedEx(LogViewSelectionModelItem * pItemFirst, LogViewSelectionModelItem * pItemSecond) const;
    void scrollSelection(bool shiftOn);
    void UpdateLock(bool updateLock){ m_updateLock = updateLock; }
public slots:
    virtual void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;
    virtual void clear() override;
    virtual void reset() override;
    virtual void clearCurrentIndex() override;

signals:
    void selectionChangedEx();

private:
    QScrollBar* m_pSlider = nullptr;
    int32_t m_offset{ -1 };
    LogViewSelectionModelItem m_selectFirst{ 0,0 };
    LogViewSelectionModelItem m_selectSecond{ 0,0 };
    bool m_updateLock = false;
};

#endif // LOGVIEWSELECTIONMODEL_H
