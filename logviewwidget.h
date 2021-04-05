/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef LOGVIEWWIDGET_H
#define LOGVIEWWIDGET_H

#include <QLineEdit>
#include <QWidget>
#include "logviewmodel.h"

class LogViewTable;
class QGestureEvent;
class QPushButton;
class QDialogButtonBox;
class QScrollBar;
class QToolBar;
class QLabel;

class LogViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewWidget(QString windowTitle, QString fileName);
    ~LogViewWidget() override;


private slots:
    void onResetZoom();
    void onUpdateTreeViewStyle();
    void onMenu();
    void onResetColumnSize();
    void onScrollbarValueChanged(int);
    void onChangeStateEvent(Qt::ApplicationState state);

private:
    void closeEvent(QCloseEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

    bool gestureEvent(QGestureEvent *event);

    void updateCtrlStyle();
    void toolTip();

    void threadWatchForZoomChange();
    void newRowPresent(bool newRow);
    void updateInfo(QString info);
    void displayRowNumber();
    void displayRowNumber2(bool bSave);
    void displayMenu();
    void displayMenu2(bool bSave);
    void displayButtons();
    void displayButtons2(bool bSave);
    void autoscrollEnable();
    void autoscrollEnable2(bool bSave);

    QDialogButtonBox* m_buttonBox = nullptr;
    //QPushButton *m_buttonResetZoom = nullptr;
    LogViewTable *m_table = nullptr;
    QScrollBar* m_scrollbar = nullptr;
    QPushButton * m_buttonDown = nullptr;
    QAction *m_actDown = nullptr;
    QAction *m_actAutoscroll = nullptr;
    QToolBar *m_toolBar = nullptr;
    QLabel *m_label = nullptr;
    bool m_buttonDownNewFlag = false;

    QString m_windowTitle;
    QString m_filename;

    bool m_bFirstShow = true;

    bool m_show = false;
    bool m_exit = false;
    bool m_threadExit = true;
    bool m_forceUpdateZoom = false;
    double m_scale = 0.0;
    std::condition_variable  m_scaleCondition;
    std::mutex               m_scaleMutex;
    QFont m_default_Font;
};

#endif // LOGVIEWWIDGET_H
