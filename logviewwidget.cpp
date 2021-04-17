/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "logviewwidget.h"

#include <QException>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QTextCodec>
#include <QMessageBox>
#include <QWindow>
#include <QScreen>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QHeaderView>
#include <QFontDatabase>
#include <QImage>
#include <QPainter>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QWheelEvent>
#include <QToolTip>
#include <QGestureEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QScroller>
#include <QToolBar>
#include <QMenu>
#include <QLabel>
#include <QSettings>

#include <chrono>
#include <utility>
#include <thread>
#include <cmath>

#include "iconhelper.h"
#include "logviewtable.h"
#include "stylehelper.h"
#include "versionhelper.h"

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

LogViewWidget::LogViewWidget(QString windowTitle, QString fileName)
    : m_windowTitle(std::move(windowTitle))
    , m_filename(std::move(fileName))
{
    DEBUGTRACE();
    m_windowTitle += " [" PRODUCTVERSIONSTRING "]";
    m_default_Font = QApplication::font();

    //setWindowIcon(IconHelper::CreateIcon("LOG", "View", font()));
    setWindowIcon(QPixmap(":/data/logview_logo.svg"));


    setWindowTitle(m_windowTitle + " [" + m_filename + "]" );

    m_scrollbar = new QScrollBar(this);
    m_scrollbar->setOrientation(Qt::Vertical);
    m_scrollbar->setMinimum(0);
    m_scrollbar->setMaximum(0);
    m_scrollbar->setSingleStep(1);
    m_scrollbar->setPageStep(10);
    m_scrollbar->setValue(0);

    m_table = new LogViewTable(m_filename, m_scrollbar, this);

    //*** Button control

    const auto buttonHorisontalPolicy = QSizePolicy::MinimumExpanding;
    const auto buttonVerticalPolicy = QSizePolicy::Expanding;

//    m_buttonResetZoom = new QPushButton(this);
//    m_buttonResetZoom->setIcon(QPixmap(":/data/b_zoom.1.svg"));
//    m_buttonResetZoom->setToolTip(tr("Reset zoom level"));
//    m_buttonResetZoom->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);

    auto buttonUp = new QPushButton(this);
    buttonUp->setIcon(QPixmap(":/data/b_Up.1.svg"));
    buttonUp->setToolTip(tr("Up"));
    buttonUp->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);
    buttonUp->setDefault(true);

    m_buttonDown = new QPushButton(this);
    m_buttonDown->setIcon(QPixmap(":/data/b_Down.1.svg"));
    m_buttonDown->setToolTip(tr("Down"));
    m_buttonDown->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);
    m_buttonDown->setDefault(true);

//    auto buttonClose = new QPushButton(this);
//    buttonClose->setIcon(QPixmap(":/data/b_close.1.svg"));
//    buttonClose->setToolTip(tr("Quit application"));
//    buttonClose->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);
//    buttonClose->setDefault(true);

//    auto buttonCopy = new QPushButton(this);
//    buttonCopy->setIcon(QPixmap(":/data/b_copy.1.svg"));
//    buttonCopy->setToolTip(tr("Copy selected text to clipboard"));
//    buttonCopy->setSizePolicy(buttonHorisontalPolicy, buttonVerticalPolicy);

#if !defined (Q_OS_ANDROID)
    //m_buttonResetZoom->setText(IconHelper::GetIconSizeInPersent() + "%");
    buttonUp->setText(tr("Up"));
    m_buttonDown->setText(tr("Down"));
    //buttonClose->setText(tr("Quit"));
    //buttonCopy->setText(tr("Copy"));
#endif // !defined (Q_OS_ANDROID)

    m_buttonBox = new QDialogButtonBox(Qt::Horizontal);
    //m_buttonBox->addButton(m_buttonResetZoom, QDialogButtonBox::ResetRole);
    m_buttonBox->addButton(buttonUp, QDialogButtonBox::ActionRole);
    m_buttonBox->addButton(m_buttonDown, QDialogButtonBox::ActionRole);

    //m_buttonBox->addButton(buttonCopy, QDialogButtonBox::ActionRole);
    //m_buttonBox->addButton(buttonClose, QDialogButtonBox::RejectRole);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->addWidget(m_table);
    horizontalLayout->addWidget(m_scrollbar);

    auto frame = new QFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);
    frame->setLayout(horizontalLayout);

    m_toolBar = new QToolBar(this);
    m_toolBar->setOrientation(Qt::Orientation::Horizontal);

    auto actionMenu = m_toolBar->addAction(QPixmap(":/data/b_menu.1.svg"), tr("Menu"));
    connect(actionMenu, &QAction::triggered, this, &LogViewWidget::onMenu);

    connect(m_toolBar->addAction(QPixmap(":/data/b_zoom.1.svg"),    tr("Reset zoom level")) , &QAction::triggered, this,    &LogViewWidget::onResetZoom);
    connect(m_toolBar->addAction(QPixmap(":/data/b_copy.1.svg"),    tr("Copy selected"))    , &QAction::triggered, m_table, &LogViewTable::onCopySelected);
    connect(m_toolBar->addAction(QPixmap(":/data/b_rownum.1.svg"),  tr("Row number"))       , &QAction::triggered, this,    &LogViewWidget::displayRowNumber);
    m_actAutoscroll = m_toolBar->addAction(QPixmap(":/data/b_autoscroll.disable.1.svg"), tr("Autoscroll"));
    connect(m_actAutoscroll                                                                 , &QAction::triggered, this,    &LogViewWidget::autoscrollEnable);
    connect(m_toolBar->addAction(QPixmap(":/data/b_Up.1.svg"),      tr("Up"))               , &QAction::triggered, m_table, &LogViewTable::onUp);
    m_actDown = m_toolBar->addAction(QPixmap(":/data/b_Down.1.svg"), tr("Down"));
    connect(m_actDown                                                                       , &QAction::triggered, m_table, &LogViewTable::onDown);

    //connect(m_toolBar->addAction(QPixmap(":/data/b_reset.1.svg"), tr("Resize column to contents")),  &QAction::triggered, this, &LogViewWidget::onResetColumnSize);

    // Spacer
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);

    m_label = new QLabel("Info");
    m_toolBar->addWidget(m_label);

    auto verticalLayout = new QVBoxLayout();
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(3, 3, 3, 3);
    verticalLayout->addWidget(m_toolBar);
    verticalLayout->addWidget(frame);
    verticalLayout->addWidget(m_buttonBox);

    setLayout(verticalLayout);

#if defined (Q_OS_ANDROID)

    auto pObj1 = this;
    pObj1->grabGesture(Qt::PinchGesture);
    pObj1->grabGesture(Qt::PanGesture);
    pObj1->grabGesture(Qt::SwipeGesture);

    auto pObj = m_table;

    QScroller::grabGesture(pObj, QScroller::LeftMouseButtonGesture);
    QScrollerProperties properties = QScroller::scroller(pObj)->scrollerProperties();
    properties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
        QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff));
    properties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
        QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff));

    QScroller::scroller(pObj)->setScrollerProperties(properties);

#endif //

    //connect(m_buttonResetZoom, &QPushButton::clicked, this,    &LogViewWidget::onResetZoom);
    connect(buttonUp,          &QPushButton::clicked, m_table, &LogViewTable::onUp);
    connect(m_buttonDown,      &QPushButton::clicked, m_table, &LogViewTable::onDown);
    //connect(buttonCopy,        &QPushButton::clicked, m_table, &LogViewTable::onCopySelected);
    //connect(buttonClose,       &QPushButton::clicked, this,    &LogViewWidget::close);

    connect(m_table,           &LogViewTable::newRowPresent, this, &LogViewWidget::newRowPresent);
    connect(m_table,           &LogViewTable::updateInfo, this, &LogViewWidget::updateInfo);

    connect(m_scrollbar,       &QScrollBar::valueChanged, this, &LogViewWidget::onScrollbarValueChanged);

    auto threadZoom = [](LogViewWidget* self) {
        try {
            self->m_threadExit = false;
            self->threadWatchForZoomChange();
        } catch(const std::exception &e) {
            LOG_ERROR_A(QString("threadZoom: %1").arg(e.what()).toStdString().c_str());
        } catch(...) {
            LOG_ERROR_A(QString("threadZoom: Unexpected exception").toStdString().c_str());
        }
        self->m_threadExit = true;
    };
    (std::thread(threadZoom, this)).detach();

    QPalette pal = palette();
    pal.setColor(QPalette::Highlight, QColor(0xC9,0xE7,0xFB));
    pal.setColor(QPalette::HighlightedText, Qt::black);
    m_table->setPalette(pal);

#if defined (Q_OS_ANDROID)
    resize(QApplication::desktop()->availableGeometry(this).size());
#else
    resize(QApplication::desktop()->availableGeometry(this).size() / 2);
#endif
    displayRowNumber2(false);
    displayMenu2(false);
    displayButtons2(false);
    autoscrollEnable2(false);

    connect(QCoreApplication::instance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(onChangeStateEvent(Qt::ApplicationState)));
}

LogViewWidget::~LogViewWidget()
{
    DEBUGTRACE();
    m_exit = true;
    m_scaleCondition.notify_all();

    int32_t timeout = 1000;
    while (!m_threadExit && timeout--)
    {
        QCoreApplication::processEvents();
        if(!m_threadExit)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void LogViewWidget::showEvent(QShowEvent *event)
{
    DEBUGTRACE();

    QWidget::showEvent(event);

    if (m_bFirstShow)
    {

        m_bFirstShow = false;

        updateCtrlStyle();

        m_table->setFocus();

#if defined (Q_OS_ANDROID)
        //showFullScreen();
#endif

    }
}

void LogViewWidget::closeEvent(QCloseEvent * e)
{
    m_exit = true;
    QWidget::closeEvent(e);
}

void LogViewWidget::wheelEvent(QWheelEvent *e)
{
    auto bControl = (e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier;

    // unlock scrollbar (if app lost focus while scrollbar was locked)
    if(!m_scrollbar->isEnabled() && !bControl)
        m_scrollbar->setEnabled(true);

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

void LogViewWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_scrollbar->isEnabled() && (event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
    {
        m_scrollbar->setEnabled(false);
        toolTip();
    }

    if (event->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(event);
}

void LogViewWidget::keyReleaseEvent(QKeyEvent *event)
{
    if(!m_scrollbar->isEnabled() && (event->modifiers() & Qt::ControlModifier) != Qt::ControlModifier)
    {
        m_scrollbar->setEnabled(true);
        QToolTip::hideText();
    }
}

bool LogViewWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}

bool LogViewWidget::gestureEvent(QGestureEvent *event)
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

void LogViewWidget::updateCtrlStyle()
{

    auto val = IconHelper::GetIconSize(m_default_Font);
    auto newFontSz = val.toInt();

#if !defined (Q_OS_ANDROID)

    const auto sc = IconHelper::GetIconScale().toDouble();

    auto fontX = font();
    const auto fontPixel = fontX.pixelSize();
    const auto fontPoint = fontX.pointSize();

    if(fontPixel > 0)
    {
        auto xTmp =  sc * 0.4 * m_default_Font.pixelSize();
        auto fontSz = static_cast<int32_t>(xTmp);
        fontX.setPixelSize(fontSz);
    }
    else if(fontPoint > 0)
    {
        auto xTmp =  sc * 0.4 * m_default_Font.pointSizeF();
        auto fontSz = xTmp;
        fontX.setPointSize(static_cast<int>(fontSz));
    }

//    qDebug() << "defF " << val;
//    QApplication::setFont(fontX);

    QFont displayFont(fontX);
    displayFont.setFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    setFont(displayFont);
    m_table->setFont(displayFont);
    m_toolBar->setFont(displayFont);
    m_label->setFont(displayFont);
    auto headerHorisontal = m_table->horizontalHeader();
    headerHorisontal->setFont(displayFont);
    QFontMetrics fm(displayFont);
#else
    QFontMetrics fm(m_default_Font);

#endif // !defined (Q_OS_ANDROID)



//    auto mod = static_cast<LogViewModel*>(m_table->model());
//    mod->setNewFont(displayFont);
    //    auto xModel = qobject_cast<LogViewModel*>(m_table->model());
    //    if (xModel)
    //        xModel->setItemSize(maxSize);

    const auto lineSpacing = fm.lineSpacing();
    const auto maxSize = std::max(lineSpacing, (newFontSz));
    const auto sectionMaxSize = QString::number(maxSize + 10);

#if defined (Q_OS_ANDROID)
    const auto sectionSize = QString::number(8);
    const auto sectionMinSize = QString::number(4);
#else
    const auto sectionSize = QString::number(maxSize / 2);
    const auto sectionMinSize = QString::number(maxSize / 4);
#endif // Q_OS_ANDROID


    setStyleSheet(
        "QTableView  {border: none;}"
        "QHeaderView::down-arrow { width:" + val + "px; height:" + val + "px; }"
        "QHeaderView::up-arrow   { width:" + val + "px; height:" + val + "px; }"
        "QHeaderView::section    { height:" + sectionMaxSize + "px; }"
        "QLineEdit               { height:" + sectionMaxSize + "px; }"
        "QPushButton             { icon-size: " + val + "px; }"
        "QToolBar                { icon-size: " + val + "px; }"
        "QScrollBar:vertical   { border: 0px solid transparent; background: transparent; width:  " + sectionSize + "px; margin: 0px 0px 0px 0px; }"
        "QScrollBar:horizontal { border: 0px solid transparent; background: transparent; height: " + sectionSize + "px; margin: 0px 0px 0px 0px; }"
        "QScrollBar::handle    { background: Deepskyblue; border: " + sectionMinSize + "px solid transparent; border-radius: " + sectionMinSize + "px; }"
        "QScrollBar::add-line:vertical  { height: 0px; subcontrol-position: bottom; subcontrol-origin: margin; }"
        "QScrollBar::sub-line:vertical  { height: 0px; subcontrol-position: top;    subcontrol-origin: margin; }"
        "QScrollBar::add-line:horizontal { width: 0px; subcontrol-position: right;  subcontrol-origin: margin; }"
        "QScrollBar::sub-line:horizontal { width: 0px; subcontrol-position: left;   subcontrol-origin: margin; }"
        );

    auto mod = static_cast<LogViewModel*>(m_table->model());
#if !defined (Q_OS_ANDROID)
    emit mod->xrowCountChanged(&displayFont);
#else
    emit mod->xrowCountChanged(&m_default_Font);
#endif // !defined (Q_OS_ANDROID)
}

void LogViewWidget::toolTip()
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
    //m_buttonResetZoom->setText(iconSizeInPersent + "%");
//#endif // defined (Q_OS_ANDROID)

}


void LogViewWidget::onUpdateTreeViewStyle()
{

    if(!m_forceUpdateZoom && std::abs(m_scale) < 0.01)
        return;

    m_forceUpdateZoom = false;

    IconHelper::GetIconSize(m_default_Font, m_scale);

    updateCtrlStyle();

    toolTip();

    m_scale = 0;
}

void LogViewWidget::threadWatchForZoomChange()
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

void LogViewWidget::newRowPresent(bool newRow)
{
    if(newRow)
    {
        if(!m_buttonDownNewFlag)
        {
            m_buttonDownNewFlag = true;
            m_buttonDown->setIcon(QPixmap(":/data/b_Down_New.1.svg"));
            m_actDown->setIcon(QPixmap(":/data/b_Down_New.1.svg"));
        }
    }
    else
    {
        if(m_buttonDownNewFlag)
        {
            m_buttonDownNewFlag = false;
            m_buttonDown->setIcon(QPixmap(":/data/b_Down.1.svg"));
            m_actDown->setIcon(QPixmap(":/data/b_Down.1.svg"));
        }
    }

}

void LogViewWidget::updateInfo(QString info)
{
    m_label->setText(info + " Zoom " + IconHelper::GetIconSizeInPersent() + "%");
}

void LogViewWidget::onResetZoom()
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

void LogViewWidget::onMenu()
{

    auto val = IconHelper::GetIconSize(font());

    auto actionsMenu = new QMenu(this);
    actionsMenu->setStyle(new StyleHelper(val.toInt()));
    actionsMenu->setMinimumHeight(this->height()/2);

    connect(actionsMenu->addAction(QPixmap(":/data/layout_menu.svg"), tr("Show menu"))
                , &QAction::triggered, this, &LogViewWidget::displayMenu);

    connect(actionsMenu->addAction(QPixmap(":/data/layout_buttons.svg"), tr("Show buttons"))
                , &QAction::triggered, this, &LogViewWidget::displayButtons);

    connect(actionsMenu->addAction(QPixmap(":/data/b_zoom.1.svg"), tr("Reset zoom level"))
                , &QAction::triggered, this, &LogViewWidget::onResetZoom);

    connect(actionsMenu->addAction(QPixmap(":/data/b_copy.1.svg"), tr("Copy selected"))
                , &QAction::triggered, m_table, &LogViewTable::onCopySelected);

    connect(actionsMenu->addAction(QPixmap(":/data/b_rownum.1.svg"), tr("Row number"))
                , &QAction::triggered, this, &LogViewWidget::displayRowNumber);

    connect(actionsMenu->addAction(QPixmap(m_table->isAutoscrollEnabled() ? ":/data/b_autoscroll.enable.1.svg" : ":/data/b_autoscroll.disable.1.svg") , tr("Autoscroll"))
                , &QAction::triggered, this, &LogViewWidget::autoscrollEnable);

//    connect(actionsMenu->addAction(QPixmap(":/data/b_reset.1.svg"), tr("Resize column to contents"))
//                , &QAction::triggered, this, &LogViewWidget::onResetColumnSize);

//    connect(actionsMenu->addAction(QPixmap(":/data/b_open1.1.svg"), tr("Open selected file"))
//                , &QAction::triggered, this, &LogViewWidget::onSelectFile);

    connect(actionsMenu->addAction(QPixmap(":/data/b_close.1.svg"), tr("Quit application"))
                , &QAction::triggered, this, &LogViewWidget::close);

    actionsMenu->exec(mapToGlobal(QPoint(2, 3)));

}

void LogViewWidget::onResetColumnSize()
{

}

void LogViewWidget::onScrollbarValueChanged(int)
{
    auto mod = static_cast<LogViewModel*>(m_table->model());
    updateInfo(mod->headerData(0, Qt::Orientation::Horizontal, Qt::DisplayRole).toString());
}

void LogViewWidget::onChangeStateEvent(Qt::ApplicationState /*state*/)
{
    //qDebug() << state;
}

void LogViewWidget::displayRowNumber()
{
    displayRowNumber2(true);
}

void LogViewWidget::displayRowNumber2(bool bSave)
{
#define KEY_DISPLAY_ROW_NUMBER "cfg/KEY_DISPLAY_ROW_NUMBER"

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    auto enableElement = settings.value(KEY_DISPLAY_ROW_NUMBER, false).toBool();

    if(bSave)
        enableElement = !enableElement;

    m_table->verticalHeader()->setVisible(enableElement);
    m_table->resize(QSize());

    if(bSave)
        settings.setValue(KEY_DISPLAY_ROW_NUMBER, enableElement);
}

void LogViewWidget::displayMenu()
{
    displayMenu2(true);
}

void LogViewWidget::displayMenu2(bool bSave)
{
#define KEY_LAYOUT_EX "cfg/KEY_LAYOUT_EX"

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    auto enableElement = settings.value(KEY_LAYOUT_EX, false).toBool();

    if(bSave)
        enableElement = !enableElement;

    for (int ind = 1; ind < m_toolBar->actions().count() -2; ind++) {
        m_toolBar->actions().at(ind)->setVisible(enableElement);
    }

    if(bSave)
        settings.setValue(KEY_LAYOUT_EX, enableElement);
}

void LogViewWidget::displayButtons()
{
    displayButtons2(true);
}

void LogViewWidget::displayButtons2(bool bSave)
{
#define KEY_LAYOUT_BUTTONBOX "cfg/KEY_LAYOUT_BUTTONBOX"

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    auto enableElement = settings.value(KEY_LAYOUT_BUTTONBOX, false).toBool();

    if(bSave)
        enableElement = !enableElement;

    m_buttonBox->setVisible(enableElement);

    if(bSave)
        settings.setValue(KEY_LAYOUT_BUTTONBOX, enableElement);

}

void LogViewWidget::autoscrollEnable()
{
    autoscrollEnable2(true);
}

void LogViewWidget::autoscrollEnable2(bool bSave)
{
#define KEY_AUTOSCROLL_ENABLE "cfg/KEY_AUTOSCROLL_ENABLE"

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    auto enableElement = settings.value(KEY_AUTOSCROLL_ENABLE, false).toBool();

    if(bSave)
        enableElement = !enableElement;
    m_table->setAutoscroll(enableElement);
    m_actAutoscroll->setIcon(QPixmap(m_table->isAutoscrollEnabled() ? ":/data/b_autoscroll.enable.1.svg" : ":/data/b_autoscroll.disable.1.svg"));

    if(bSave)
        settings.setValue(KEY_AUTOSCROLL_ENABLE, enableElement);
}

//void LogViewWidget::SetControlSize(bool bAndroid)
//{
//    DEBUGTRACE();

//    auto bValidGeom = false;
//    auto tmpWindow = window();
//    if (tmpWindow)
//    {
//        auto tmpWindowHandle = tmpWindow->windowHandle();
//        if (tmpWindowHandle)
//        {
//            auto tmpScreen = tmpWindowHandle->screen();
//            auto w = tmpScreen->availableSize().width();
//            auto h = tmpScreen->availableSize().height();

//            if (!bAndroid)
//            {
//                w /= 2;
//                h /= 2;
//            }

//            if (w > 0 && h > 0)
//            {
//                resize(w, h);
//                bValidGeom = true;
//            }
//        }
//    }

//    if (!bValidGeom)
//    {
//        auto rect = QGuiApplication::primaryScreen()->geometry();
//        auto w = rect.width();
//        auto h = rect.height();

//        if (!bAndroid)
//        {
//            w /= 2;
//            h /= 2;
//        }

//        if (w > 0 && h > 0)
//        {
//            resize(w, h);
//            bValidGeom = true;
//        }
//    }

//    if (!bValidGeom && !bAndroid)
//        showFullScreen();
//}
