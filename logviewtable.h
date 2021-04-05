/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef LOGVIEWTABLE_H
#define LOGVIEWTABLE_H

#include <QTableView>

class QEvent;
class QPaintEvent;
class QPainter;
class QResizeEvent;
class QScrollBar;

class LogViewTable : public QTableView
{
    Q_OBJECT
public:
    explicit LogViewTable(const QString &path, QScrollBar *scrollBar, QWidget *parent = nullptr);
    void setAutoscroll(bool bEnable);
    bool isAutoscrollEnabled();

protected:
    virtual bool event(QEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void updateTreeView();

    void drawCell(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index);
    bool useScrollbar(int keyX);
    QScrollBar *m_pVerticalScrollBar = nullptr;
    int32_t m_lastRow = 0;
    int32_t m_visibleRow = 0;
    bool m_autoscroll = false;
signals:
    void newRowPresent(bool);
    void updateInfo(QString);

public slots:
    void onSelectionChange();
    void onCopySelected();
    void onUp();
    void onDown();
    void onScrollbarValueChanged(int32_t scrollbarPosition);
    void onColumnWidthChanged(int colWidth);
    void onXrowCountChanged(QFont *fontVal);

};

#endif // LOGVIEWTABLE_H
