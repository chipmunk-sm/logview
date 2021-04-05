/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselector.h"
#include "fileselectorlineedit.h"
#include "fileselectorlocations.h"
#include "fileselectorstandardmodel.h"
#include "iconhelper.h"
#include "stylehelper.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QScroller>
#include <QGestureEvent>
#include <QHeaderView>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QToolTip>
#include <QMenuBar>
#include <QToolBar>
#include <QSettings>
#include <QTreeView>

#include <thread>
#include <cmath>
#include <vector>

#ifndef LOG_ERROR_A
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define LOG_ERROR_A(mess) qFatal("%s", mess)
#   define LOG_WARNING_A(mess) qWarning("%s", mess)
#endif // LOG_ERROR_A

#if 1
#   ifndef QDEBUG_H
#      include <QDebug>
#   endif
#   define DEBUGTRACE() qDebug() << Q_FUNC_INFO
#else
#   define DEBUGTRACE()
#endif

FileSelector::FileSelector(bool saveFileMde, const QString & rootPath, const QString defaultName, QWidget * parent)
    : QWidget(parent)
{
    setWindowIcon(QPixmap(":/data/logview_logo.svg"));

    m_selectedName = defaultName;
    m_saveMode = saveFileMde;

    m_default_Font = QApplication::font();

    //setWindowIcon(IconHelper::CreateIcon("LOG", "select", font()));

    //*** FileSelectorLineEdit field
    m_lineEdit = new FileSelectorLineEdit(this);
    m_lineEdit->setPlaceholderText(QString("Path to file"));
    connect(m_lineEdit, &QLineEdit::textChanged, this, &FileSelector::onTextChanged);

    //*** FileSelector
    m_fileSelector = std::make_shared<QTreeView>(this);
    m_fileSelector->setRootIsDecorated(true);
    m_fileSelector->setSelectionMode(QTreeView::SingleSelection);
    m_fileSelector->setAnimated(true);
    m_fileSelector->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_fileSelector->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_fileSelector->setSortingEnabled(false);
    m_fileSelector->setAlternatingRowColors(true);
    m_fileSelector->header()->setSortIndicator(0, Qt::AscendingOrder);
    m_fileSelector->header()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_fileSelector->header()->setSectionsClickable(true);
    m_fileSelector->header()->setHighlightSections(false);
    m_fileSelector->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto modelX = new FileSelectorStandardModel(m_fileSelector.get());

    QPalette pal = palette();
    pal.setColor(QPalette::Highlight, QColor(0xC9,0xE7,0xFB));
    pal.setColor(QPalette::HighlightedText, Qt::black);
    m_fileSelector->setPalette(pal);

    m_fileSelector->setModel(modelX);
    connect(m_fileSelector.get(), &QTreeView::clicked, this, &FileSelector::onItemClicked);
    connect(m_fileSelector.get(), &QTreeView::doubleClicked, this, &FileSelector::onItemDoubleClicked);
    connect(m_fileSelector->header(), &QHeaderView::sectionClicked, this, &FileSelector::onResetColumnSize);

    //*** Button control
    const auto buttonHorisontalPolicy = QSizePolicy::Fixed;
    const auto buttonVerticalPolicy = QSizePolicy::Expanding;

//# if !defined (Q_OS_ANDROID)
    auto buttonClose = new QPushButton();
    buttonClose->setIcon(QPixmap(":/data/b_close.1.svg"));
    buttonClose->setToolTip(tr("Quit application"));
    buttonClose->setDefault(true);
    buttonClose->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);
    connect(buttonClose, &QPushButton::clicked, this, &FileSelector::shutdown);
//# endif

    m_buttonSelect = new QPushButton();
    m_buttonSelect->setIcon(QPixmap(":/data/b_open1.1.svg"));
    m_buttonSelect->setToolTip(tr("Select"));
    m_buttonSelect->setEnabled(false);
    m_buttonSelect->setAutoDefault(false);
    m_buttonSelect->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);
    connect(m_buttonSelect, &QPushButton::clicked, this, &FileSelector::onSelectFile);

#if !defined (Q_OS_ANDROID)
    buttonClose->setText(tr("Quit"));
    m_buttonSelect->setText(tr("Select"));
#endif // !defined (Q_OS_ANDROID)

    m_buttonBox = new QDialogButtonBox(Qt::Horizontal);
    m_buttonBox->addButton(m_buttonSelect, QDialogButtonBox::AcceptRole);
    m_buttonBox->addButton(buttonClose, QDialogButtonBox::RejectRole);

    //*** createMenu
    m_toolBar = new QToolBar(this);
    m_toolBar->setOrientation(Qt::Orientation::Horizontal);

    auto actionMenu = m_toolBar->addAction(QPixmap(":/data/b_menu.1.svg"), tr("Menu"));
    connect(actionMenu, &QAction::triggered, this, &FileSelector::onMenu);

    connect(m_toolBar->addAction(QPixmap(":/data/b_zoom.1.svg"),  tr("Reset zoom level")),           &QAction::triggered, this, &FileSelector::onResetZoom);
    connect(m_toolBar->addAction(QPixmap(":/data/b_reset.1.svg"), tr("Resize column to contents")),  &QAction::triggered, this, &FileSelector::onResetColumnSize);


    //*** Layout
    auto verticalLayout = new QVBoxLayout();
    verticalLayout->setSpacing(5);
    verticalLayout->setContentsMargins(3, 3, 3, 3);

    verticalLayout->addWidget(m_toolBar);
    verticalLayout->addWidget(m_lineEdit);
    verticalLayout->addWidget(m_fileSelector.get());
    verticalLayout->addWidget(m_buttonBox);

    setLayout(verticalLayout);

    displayMenu2(false);
    displayBottons2(false);

#if /*1 ||*/ defined (Q_OS_ANDROID)

    auto pObj1 = this;
    pObj1->grabGesture(Qt::PinchGesture);
    pObj1->grabGesture(Qt::PanGesture);
    pObj1->grabGesture(Qt::SwipeGesture);

    auto pObj = m_fileSelector.get();

    QScroller::grabGesture(pObj, QScroller::LeftMouseButtonGesture);

    QScrollerProperties properties = QScroller::scroller(pObj)->scrollerProperties();
    properties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
        QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff));
    properties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
        QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff));
    QScroller::scroller(pObj)->setScrollerProperties(properties);

#endif //


    if(m_threadExit)
    {
        auto threadWatchdog = [](FileSelector* self) {
            try {
                self->m_threadExit = false;
                self->watchdogThread();
            } catch(const std::exception &e) {
                LOG_ERROR_A(QString("threadZoom: %1").arg(e.what()).toStdString().c_str());
            } catch(...) {
                LOG_ERROR_A(QString("threadZoom: Unexpected exception").toStdString().c_str());
            }
            self->m_threadExit = true;
        };
        (std::thread(threadWatchdog, this)).detach();
    }

#if defined (Q_OS_ANDROID)
    resize(QApplication::desktop()->availableGeometry(this).size());
#else
    resize(QApplication::desktop()->availableGeometry(this).size() / 2);
#endif

    auto tmpPath = QDir::cleanPath(rootPath);
    if(QFileInfo::exists(tmpPath))
    {
        m_selectedPath = tmpPath;
        return;
    }

    //search first valid path
    while (!tmpPath.isEmpty())
    {
        QFileInfo checkPath(tmpPath);
        if (checkPath.exists())
        {
            m_lineEdit->setText(tmpPath);
            setWindowTitle(QObject::tr("Select "));
            break;
        }
        tmpPath = checkPath.dir().path();
    }

    m_lineEdit->setText(tmpPath);
    m_lineEdit->setFocus();

}

FileSelector::~FileSelector()
{
    shutdown();
}

void FileSelector::shutdown()
{
 //   qDebug() << QTime::currentTime().toString("hh:mm:ss,zzz ") << "shutdown";

    m_exit = true;
    m_scaleCondition.notify_all();

    int32_t timeout = 1000;
    while (!m_threadExit && timeout--) {
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    m_shutdown = true;
}

void FileSelector::closeEvent(QCloseEvent * e)
{
    QWidget::closeEvent(e);
    shutdown();
}

void FileSelector::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if (m_fileSelector && !m_show)
    {
        m_show = true;

        const auto widthX = m_fileSelector->width();
        if (widthX > 0)
        {
#if 1
            auto colWidth = (widthX / 5);
            m_fileSelector->setColumnWidth(0, colWidth * 4);
            m_fileSelector->setColumnWidth(1, colWidth);
            m_fileSelector->setColumnWidth(2, colWidth);
            m_fileSelector->setColumnWidth(3, colWidth * 2);
#else
            m_fileSelector->resizeColumnToContents(0);
            m_fileSelector->resizeColumnToContents(1);
            m_fileSelector->resizeColumnToContents(2);
            m_fileSelector->resizeColumnToContents(3);
#endif

        }
        updateTreeViewStyle(IconHelper::GetIconSize(m_default_Font));
    }
}

void FileSelector::wheelEvent(QWheelEvent *e)
{
    auto bControl = (e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier;

    // unlock scrollbar (if app lost focus while scrollbar was locked)
    if(!m_fileSelector->verticalScrollBar()->isEnabled() && !bControl)
        m_fileSelector->verticalScrollBar()->setEnabled(true);

    if (bControl)
    {
        const auto numPixels = e->pixelDelta();
        const auto numDegrees = e->angleDelta();

        if (!numPixels.isNull())
            m_scale = (numPixels.y() < 0.0) ? -0.1 : 0.1;
        else if (!numDegrees.isNull())
            m_scale = (numDegrees.y() < 0.0) ? -0.1 : 0.1;

        if(std::abs(m_scale) > 0.01)
            m_scaleCondition.notify_one();

        e->accept();
        return;
    }
    e->ignore();
}

void FileSelector::keyPressEvent(QKeyEvent *event)
{
    if (m_fileSelector->verticalScrollBar()->isEnabled() && (event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
    {
        m_fileSelector->verticalScrollBar()->setEnabled(false);
        toolTip();
    }

    QWidget::keyPressEvent(event);
}

void FileSelector::keyReleaseEvent(QKeyEvent *event)
{
    if(!m_fileSelector->verticalScrollBar()->isEnabled() && (event->modifiers() & Qt::ControlModifier) != Qt::ControlModifier)
    {
        m_fileSelector->verticalScrollBar()->setEnabled(true);
        QToolTip::hideText();
    }
    if (event->key() == Qt::Key_Escape)
        shutdown();
    else
        QWidget::keyReleaseEvent(event);
}

bool FileSelector::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(dynamic_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}

bool FileSelector::gestureEvent(QGestureEvent *event)
{
    auto pinch = event->gesture(Qt::PinchGesture);
    if (pinch)
    {
        auto gesture = static_cast<QPinchGesture *>(pinch);
        QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
        if ((changeFlags & QPinchGesture::ScaleFactorChanged) == QPinchGesture::ScaleFactorChanged)
        {
            double scaleFactor = gesture->scaleFactor() - 1.0;
            if (scaleFactor >= 0.01)
                m_scale = 0.1;
            else if (scaleFactor <= -0.01)
                m_scale = -0.1;

            if(std::abs(m_scale) > 0.01)
                m_scaleCondition.notify_one();
        }
    }
    return true;
}

void FileSelector::onSelectFile()
{

    if(m_selectedPath.isEmpty())
        return;
    shutdown();
}

void FileSelector::onItemClicked(const QModelIndex &current)
{
    m_lineEdit->setText(QDir::toNativeSeparators(QDir::cleanPath(current.data(FileSelectorStandardModel::Role::UrlStringRole).toString())));
}

void FileSelector::onItemDoubleClicked(const QModelIndex &current)
{
    m_lineEdit->setText(QDir::toNativeSeparators(QDir::cleanPath(current.data(FileSelectorStandardModel::Role::UrlStringRole).toString())));

    onSelectFile();
}

QString FileSelector::exec()
{

    show();

    while (!m_shutdown)
    {
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    FileSelectorLocations::AddRecentLocations(m_selectedPath, false);
    return m_selectedPath;
}

QString FileSelector::getPath()
{
    return m_selectedPath;
}

void FileSelector::displayMenu()
{
    displayMenu2(true);
}

void FileSelector::displayMenu2(bool bSave)
{

#define KEY_LAYOUT_EX "cfg/KEY_LAYOUT_EX0"

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    auto enableElement = settings.value(KEY_LAYOUT_EX, false).toBool();

    if(bSave)
        enableElement = !enableElement;

    for (int ind = 1; ind < m_toolBar->actions().count(); ind++) {
        m_toolBar->actions().at(ind)->setVisible(enableElement);
    }

    if(bSave)
        settings.setValue(KEY_LAYOUT_EX, enableElement);
}

void FileSelector::displayBottons()
{
    displayBottons2(true);
}

void FileSelector::displayBottons2(bool bSave)
{

#define KEY_LAYOUT_BUTTONBOX0 "cfg/KEY_LAYOUT_BUTTONBOX0"

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    auto enableElement = settings.value(KEY_LAYOUT_BUTTONBOX0, false).toBool();

    if(bSave)
        enableElement = !enableElement;

    m_buttonBox->setVisible(enableElement);

    if(bSave)
        settings.setValue(KEY_LAYOUT_BUTTONBOX0, enableElement);
}

void FileSelector::updateTreeViewStyle(const QString & val)
{

    QFontMetrics fm(m_default_Font);

    const auto maxSize = std::max(fm.height()
                                  //+ fm.lineSpacing()
                                  , val.toInt());
    const auto sectionMaxSize = QString::number(maxSize);

#if defined (Q_OS_ANDROID)
    const auto sectionSize = QString::number(8);
    const auto sectionMinSize = QString::number(4);
#else
    const auto sectionSize = QString::number(maxSize / 2);
    const auto sectionMinSize = QString::number(maxSize / 4);
#endif // Q_OS_ANDROID

    setStyleSheet("QTreeView { qproperty-indentation: " + val + ";"
        "height: " + val + " px;"
        "icon-size: " + val + "px;}"
        "QHeaderView::down-arrow { width:" + val + "px; height:" + val + "px; }"
        "QHeaderView::up-arrow   { width:" + val + "px; height:" + val + "px; }"
        "QHeaderView::section    { height:" + sectionMaxSize + "px; padding: 0px; }"
        "QLineEdit               { height:" + sectionMaxSize + "px; }"
        "QPushButton             { icon-size: " + val + "px; }"
        "QScrollBar:vertical   { border: 0px solid transparent; background: transparent; width:  " + sectionSize + "px; margin: 0px 0px 0px 0px; }"
        "QScrollBar:horizontal { border: 0px solid transparent; background: transparent; height: " + sectionSize + "px; margin: 0px 0px 0px 0px; }"
        "QScrollBar::handle    { background: Deepskyblue; border: " + sectionMinSize + "px solid transparent; border-radius: " + sectionMinSize + "px; }"
        "QScrollBar::add-line:vertical  { height: 0px; subcontrol-position: bottom; subcontrol-origin: margin; }"
        "QScrollBar::sub-line:vertical  { height: 0px; subcontrol-position: top;    subcontrol-origin: margin; }"
        "QScrollBar::add-line:horizontal { width: 0px; subcontrol-position: right;  subcontrol-origin: margin; }"
        "QScrollBar::sub-line:horizontal { width: 0px; subcontrol-position: left;   subcontrol-origin: margin; }"
        "QToolBar{ icon-size: " + val + "px; }"
        );

    //"QTreeView { show-decoration-selected: 1;}"
    //"QTreeView::item { border: 1px solid #d9d9d9; border-top-color: transparent; border-bottom-color: transparent; }"
    //"QTreeView::item:hover { border: 1px solid #567dbc; }"
    //"QTreeView::branch:hover { border: 1px solid #567dbc; }"
    //"QTreeView::item:selected { background-color: Lightcyan; color: black; }"
    //"QTreeView::branch:selected { border: 1px solid #567dbc; }"
    //"QTreeView::item:selected:active{  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6ea1f1, stop: 1 #567dbc); }"
    //"QTreeView::item:selected:!active { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6b9be8, stop: 1 #577fbf); }"
    //"QTreeView::branch:has-siblings:!adjoins-item {  border-image: url(vline.png) 0;}"
    //"QTreeView::branch:has-siblings:adjoins-item {  border-image: url(branch-more.png) 0;}"
    //"QTreeView::branch:!has-children:!has-siblings:adjoins-item { border-image: url(branch-end.png) 0; }"
    //"QTreeView::branch:has-children:!has-siblings:closed, QTreeView::branch:closed:has-children:has-siblings {  border-image: none;  image: url(branch-closed.png);}"
    //"QTreeView::branch:open:has-children:!has-siblings, QTreeView::branch:open:has-children:has-siblings  { border-image: none; image: url(branch-open.png);}"
    //"QTreeView::branch { background: palette(base); }"
    //"QTreeView::branch:has-siblings:!adjoins-item { background: cyan; }"
    //"QTreeView::branch:has-siblings:adjoins-item { background: red; }"
    //"QTreeView::branch:!has-children:!has-siblings:adjoins-item { background: blue; }"
    //"QTreeView::branch:closed:has-children:has-siblings { background: pink; }"
    //"QTreeView::branch:has-children:!has-siblings:closed { background: gray; }"
    //"QTreeView::branch:open:has-children:has-siblings { background: magenta;}"
    //"QTreeView::branch:open:has-children:!has-siblings { background: green;}"
    //"QScrollBar:horizontal   { height:" + sectionSize + "px; }"
    //"QScrollBar:vertical     { width:" + sectionSize + "px; } "
    //"QScrollBar::horizontal:hover { background: green; }"
    //"QScrollBar::vertical:hover   { background: red; }"
    //"QScrollBar::handle:vertical             { }"
    //"QScrollBar::handle:vertical:pressed     { }"
    //"QScrollBar::add-page:vertical           { }"
    //"QScrollBar::sub-line:vertical           { }"
    //"QScrollBar::add-line:vertical           { }"
    //"QScrollBar::up-arrow:vertical           { }"
    //"QScrollBar::up-arrow:vertical:hover     { }"
    //"QScrollBar::up-arrow:vertical:pressed   { }"
    //"QScrollBar::down-arrow:vertical         { }"
    //"QScrollBar::down-arrow:vertical:hover   { }"
    //"QScrollBar::down-arrow:vertical:pressed { }"
}

void FileSelector::toolTip()
{
    if(width() < 1 || height() < 1)
        return;

    const auto iconSizeInPersent =  IconHelper::GetIconSizeInPersent();

//#if defined (Q_OS_ANDROID)
    const auto toolTipText = QString("<center><b>Zoom %1%</b></center>").arg(iconSizeInPersent);
    auto fm = fontMetrics();
    auto boundingRect = fm.boundingRect(QRect(0,0, width(), height()), Qt::TextWordWrap, toolTipText);
    boundingRect.setWidth( boundingRect.width());
    boundingRect.setHeight(boundingRect.height());

    auto xPos =  width() - boundingRect.width()/2;
    auto yPos =  height() - boundingRect.height()/2;

    if(xPos < 1 || yPos < 1)
        return;

    xPos /= 2;
    yPos /= 2;

    QToolTip::showText(this->mapToGlobal(QPoint(xPos, yPos)), toolTipText, this);
//#else
//    m_buttonResetZoom->setText(iconSizeInPersent + "%");
//#endif // defined (Q_OS_ANDROID)

}

void FileSelector::onUpdateTreeViewStyle()
{

    if(!m_forceUpdateZoom && std::abs(m_scale) < 0.01)
        return;

    m_forceUpdateZoom = false;

    auto xModel = qobject_cast<FileSelectorStandardModel*>(m_fileSelector->model());
    if (xModel)
        xModel->RepaintMode(true);

    updateTreeViewStyle(IconHelper::GetIconSize(m_default_Font, m_scale));

    toolTip();

    m_scale = 0;

    if (xModel)
        xModel->RepaintMode(false);

}

void FileSelector::watchdogThread()
{
    while (!m_exit)
    {
        std::unique_lock<std::mutex> locker(m_scaleMutex);
        m_scaleCondition.wait(locker);
        if(m_exit)
            return;
        if(std::abs(m_scale) < 0.01)
            continue;

        QMetaObject::invokeMethod(this, "onUpdateTreeViewStyle");
    }
}

void FileSelector::onResetZoom()
{
    IconHelper::ResetIconSize();
    m_forceUpdateZoom = true;
    QMetaObject::invokeMethod(this, "onUpdateTreeViewStyle");
#if defined (Q_OS_ANDROID)
    auto sz = QApplication::desktop()->availableGeometry(this).size();
    if(sz.width() < width() || sz.height() < height())
        resize(sz);
#endif

}

void FileSelector::onResetColumnSize()
{
    auto items = m_fileSelector->header()->count();
    for (auto ind = 0; ind < items;ind++)
    {
        m_fileSelector->resizeColumnToContents(ind);
    }
}

void FileSelector::onTextChanged(const QString & path)
{

    m_selectedPath = "";

    if (!path.isEmpty())
    {
        QFileInfo checkPath(path);
        if (checkPath.exists() && (m_saveMode || checkPath.isFile()))
            m_selectedPath = path;
    }

    if(m_buttonSelect)
    {
        m_buttonSelect->setIcon(m_selectedPath.isEmpty() ? QPixmap(":/data/b_open1.1.svg") : QPixmap(":/data/b_open2.1.svg"));
        m_buttonSelect->setEnabled(!m_selectedPath.isEmpty());
    }

    //m_buttonSelect->setVisible(!tmpPath.isEmpty());
}

void FileSelector::onMenu()
{

    auto val = IconHelper::GetIconSize(font());

    auto actionsMenu = new QMenu(this);
    actionsMenu->setStyle(new StyleHelper(val.toInt()));
    actionsMenu->setMinimumHeight(this->height()/2);

    connect(actionsMenu->addAction(QPixmap(":/data/layout_menu.svg"), tr("Show menu"))
                , &QAction::triggered, this, &FileSelector::displayMenu);

    connect(actionsMenu->addAction(QPixmap(":/data/layout_buttons.svg"), tr("Show buttons"))
                , &QAction::triggered, this, &FileSelector::displayBottons);

    connect(actionsMenu->addAction(QPixmap(":/data/b_zoom.1.svg"), tr("Reset zoom level"))
                , &QAction::triggered, this, &FileSelector::onResetZoom);

    connect(actionsMenu->addAction(QPixmap(":/data/b_reset.1.svg"), tr("Resize column to contents"))
                , &QAction::triggered, this, &FileSelector::onResetColumnSize);

    auto act = actionsMenu->addAction(m_selectedPath.isEmpty()? QPixmap(":/data/b_open1.1.svg") : QPixmap(":/data/b_open2.1.svg"), tr("Select"));
    if(m_selectedPath.isEmpty())
        act->setEnabled(false);
    connect(act, &QAction::triggered , this, &FileSelector::onSelectFile);

    connect(actionsMenu->addAction(QPixmap(":/data/b_close.1.svg"), tr("Clear recent locations"))
                , &QAction::triggered, &FileSelectorLocations::ClearRecentLocations);

    connect(actionsMenu->addAction(QPixmap(":/data/b_close.1.svg"), tr("Quit application"))
                , &QAction::triggered, this, &FileSelector::shutdown);

    actionsMenu->exec(mapToGlobal(QPoint(2, 3)));

}
