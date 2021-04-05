/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTOR_H
#define FILESELECTOR_H

#include <QWidget>

#include <mutex>
#include <condition_variable>

class QPushButton;
class QTreeView;
class QGestureEvent;
class FileSelectorLineEdit;
class QDialogButtonBox;
class QMenuBar;
class QToolBar;

class FileSelector : public QWidget
{
    Q_OBJECT
public:
    explicit FileSelector(bool saveFileMde, const QString & rootPath, const QString defaultName, QWidget * parent = nullptr);
    ~FileSelector() override;
    QString exec();
    QString getPath();
    void displayMenu();
    void displayMenu2(bool bChange);

    void displayBottons();
    void displayBottons2(bool bSave);
private slots:
    void onSelectFile();
    void onItemClicked(const QModelIndex &current);
    void onItemDoubleClicked(const QModelIndex &current);
    void onUpdateTreeViewStyle();
    void onResetZoom();
    void onResetColumnSize();
    void onTextChanged(const QString &path);
    void onMenu();

private:
    void closeEvent(QCloseEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

    bool gestureEvent(QGestureEvent *event);
    void updateTreeViewStyle(const QString &val);
    void toolTip();

    void watchdogThread();
    void shutdown();

    bool m_show = false;
    bool m_exit = false;
    bool m_threadExit = true;
    bool m_shutdown = false;

    bool    m_saveMode = false;
    QString m_selectedName;
    QString m_selectedPath;

    QPushButton *m_buttonSelect = nullptr;
    std::shared_ptr<QTreeView> m_fileSelector = nullptr;
    QToolBar *m_toolBar = nullptr;
    FileSelectorLineEdit *m_lineEdit = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;

    bool m_forceUpdateZoom = false;
    double m_scale = 0.0;
    std::condition_variable  m_scaleCondition;
    std::mutex               m_scaleMutex;
    QFont m_default_Font;
};


#endif // FILESELECTOR_H
