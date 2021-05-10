/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "logviewtable.h"
#include "logviewmodel.h"
#include "logviewselectionmodel.h"

#include <QResizeEvent>
#include <QHeaderView>
#include <QBitArray>
#include <QPainter>
#include <QEvent>
#include <QPen>
#include <QScrollBar>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QGestureEvent>
#include <QScroller>
#include <QToolTip>

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

#include <QHeaderView>

LogViewTable::LogViewTable(const QString &path, QScrollBar *scrollBar, QWidget *parent)
    : QTableView(parent)
    , m_pVerticalScrollBar(scrollBar)
{
    DEBUGTRACE();

    auto model = new LogViewModel(this);

    connect(scrollBar, &QScrollBar::valueChanged, this, &LogViewTable::onScrollbarValueChanged);

    connect(model, &LogViewModel::xrowCountChanged,    this, &LogViewTable::onXrowCountChanged);
    connect(model, &LogViewModel::columnWidthChanged, this, &LogViewTable::onColumnWidthChanged);

    setModel(model);

    auto selectionModel = new LogViewSelectionModel(model, scrollBar, this);
    connect(selectionModel, &LogViewSelectionModel::selectionChangedEx, this, &LogViewTable::onSelectionChange);
    setSelectionModel(selectionModel);

    setDragDropOverwriteMode(false);
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setContextMenuPolicy(Qt::NoContextMenu);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
    setAlternatingRowColors(true);
    setWordWrap(false);
    setCornerButtonEnabled(true);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    auto headerHorisontal = horizontalHeader();
    headerHorisontal->setSectionResizeMode(QHeaderView::Fixed);
    headerHorisontal->setDefaultAlignment(Qt::AlignVCenter);
    headerHorisontal->hide();

    auto headerVertical = verticalHeader();
    headerVertical->setSectionResizeMode(QHeaderView::ResizeToContents);
    headerVertical->setDefaultAlignment(Qt::AlignTop);
    headerVertical->hide();

    try
    {
        model->OpenFile(path);
    }
    catch (std::exception  const&e)
    {
        QMessageBox::critical(this, QObject::tr("Open log"), QObject::tr(e.what()), QMessageBox::Ok);
    }
    catch (...)
    {
        QMessageBox::critical(this, QObject::tr("Open log"), QObject::tr("Unexpected exception"), QMessageBox::Ok);
    }

}

bool LogViewTable::event(QEvent *event)
{
    //DEBUGTRACE();

    if (event->type() == QEvent::Wheel)
        return m_pVerticalScrollBar->event(event);

    if (event->type() ==  QEvent::ScrollPrepare)
    {
        auto prepEvent = static_cast<QScrollPrepareEvent *>(event);
        QScrollBar *scrollHor = horizontalScrollBar();
        prepEvent->setViewportSize(QSizeF(size()));//QSizeF(viewport()->size())
        prepEvent->setContentPosRange(QRectF(0, 0, scrollHor->maximum(),
             static_cast<double>(m_pVerticalScrollBar->maximum()) * rowHeight(0)));
        prepEvent->setContentPos(QPointF(scrollHor->value(),
             static_cast<double>(m_pVerticalScrollBar->value()) * rowHeight(0)));
        prepEvent->accept();
        return true;
    }

    if (event->type() ==  QEvent::Scroll)
    {
        auto scrollEvent = static_cast<QScrollEvent *>(event);
        horizontalScrollBar()->setValue(static_cast<int>(scrollEvent->contentPos().x()));
        m_pVerticalScrollBar->setValue(static_cast<int>(scrollEvent->contentPos().y() / rowHeight(0)));
        return true;
    }

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
    {
        auto evt = static_cast<QKeyEvent*>(event);

        if (evt->type() == QEvent::KeyPress &&
                evt->key() == Qt::Key_C &&
                (evt->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
        {
            onCopySelected();
            return true;
        }

        switch (evt->key())
        {
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Down:
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        {
            if (useScrollbar(evt->key()))
            {
                m_pVerticalScrollBar->event(event);
                static_cast<LogViewSelectionModel*>(selectionModel())->scrollSelection((evt->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier);
                return true;
            }
        }
        break;
        default:
            break;
        }
    }
    return QTableView::event(event);
}

bool LogViewTable::useScrollbar(int keyX)
{
    DEBUGTRACE();
    const auto bottom = model()->rowCount() - 1;
    if (bottom < 0)
        return false;

    const auto current = currentIndex();
    if (!current.isValid())
        return false;

    auto currentRow = current.row();
    if (keyX == Qt::Key_Down || keyX == Qt::Key_PageDown)
    {
        return currentRow == bottom;
    }

    if (keyX == Qt::Key_Up || keyX == Qt::Key_PageUp)
    {
        return currentRow == 0;
    }

    return keyX == Qt::Key_Home || keyX == Qt::Key_End;
}

void LogViewTable::setAutoscroll(bool bEnable)
{
    m_autoscroll = bEnable;
}

void LogViewTable::resizeEvent(QResizeEvent *event)
{
    DEBUGTRACE();
    QTableView::resizeEvent(event);
    auto mod = static_cast<LogViewModel*>(model());
    emit mod->xrowCountChanged(nullptr);
}

void LogViewTable::paintEvent(QPaintEvent *event)
{

    const auto verticalHeader = QTableView::verticalHeader();
    const auto horizontalHeader = QTableView::horizontalHeader();

    if (horizontalHeader->count() == 0 || verticalHeader->count() == 0)
        return;

#if (QT_VERSION_MAJOR < 6)
    auto option = viewOptions();
#else
    QStyleOptionViewItem option;
    initViewItemOption(&option);
#endif
    const auto offset = dirtyRegionOffset();
    const auto showGrid = QTableView::showGrid();
    const auto gridSize = showGrid ? 1 : 0;
    const auto gridHint = style()->styleHint(QStyle::SH_Table_GridLineColor, &option, this);
    const auto gridColor = static_cast<QColor>(static_cast<QRgb>(gridHint));
    const auto gridPen = QPen(gridColor, 0, gridStyle());
    const auto alternate = alternatingRowColors();

    QPainter painter(viewport());

    auto x = horizontalHeader->length() - horizontalHeader->offset();
    auto y = verticalHeader->length() - verticalHeader->offset() - 1;

    auto firstVisualRow = qMax(verticalHeader->visualIndexAt(0), 0);
    auto lastVisualRow = verticalHeader->visualIndexAt(verticalHeader->viewport()->height());

    if (lastVisualRow == -1)
        lastVisualRow = model()->rowCount() - 1;

    auto firstVisualColumn = horizontalHeader->visualIndexAt(0);
    auto lastVisualColumn = horizontalHeader->visualIndexAt(horizontalHeader->viewport()->width());

    if (firstVisualColumn == -1)
        firstVisualColumn = 0;

    if (lastVisualColumn == -1)
        lastVisualColumn = horizontalHeader->count() - 1;

    QBitArray drawn((lastVisualRow - firstVisualRow + 1) * (lastVisualColumn - firstVisualColumn + 1));

    const QRegion region = event->region().translated(offset);

    for (QRect dirtyArea : region)
    {

        dirtyArea.setBottom(qMin(dirtyArea.bottom(), int(y)));
        dirtyArea.setRight(qMin(dirtyArea.right(), int(x)));

        auto left = horizontalHeader->visualIndexAt(dirtyArea.left());
        auto right = horizontalHeader->visualIndexAt(dirtyArea.right());

        if (left == -1)
            left = 0;

        if (right == -1)
            right = horizontalHeader->count() - 1;

        auto bottom = verticalHeader->visualIndexAt(dirtyArea.bottom());

        if (bottom == -1)
            bottom = verticalHeader->count() - 1;

        auto top = verticalHeader->visualIndexAt(dirtyArea.top());
        auto alternateBase = (top & 1) && alternate;

        if (top == -1 || top > bottom)
            continue;

        // row
        for (auto visualRowIndex = top; visualRowIndex <= bottom; ++visualRowIndex)
        {

            auto row = verticalHeader->logicalIndex(visualRowIndex);

            if (verticalHeader->isSectionHidden(row))
                continue;

            auto rowY = rowViewportPosition(row) + offset.y();
            auto rowh = rowHeight(row) - gridSize;

            // column
            for (int visualColumnIndex = left; visualColumnIndex <= right; ++visualColumnIndex)
            {
                auto currentBit =
                    (visualRowIndex - firstVisualRow) * (lastVisualColumn - firstVisualColumn + 1)
                    + visualColumnIndex - firstVisualColumn;

                if (currentBit < 0 || currentBit >= drawn.size() || drawn.testBit(currentBit))
                    continue;

                drawn.setBit(currentBit);

                auto col = horizontalHeader->logicalIndex(visualColumnIndex);

                if (horizontalHeader->isSectionHidden(col))
                    continue;

                auto colp = columnViewportPosition(col) + offset.x();
                auto colw = columnWidth(col) - gridSize;

                const QModelIndex index = model()->index(row, col);
                if (index.isValid())
                {
                    option.rect = QRect(colp, rowY, colw, rowh);
                    if (alternate)
                    {
                        if (alternateBase)
                            option.features |= QStyleOptionViewItem::Alternate;
                        else
                            option.features &= ~QStyleOptionViewItem::Alternate;
                    }
                    drawCell(&painter, option, index);
                }
            }
            alternateBase = !alternateBase && alternate;
        }

        if (showGrid)
        {

            while (verticalHeader->isSectionHidden(verticalHeader->logicalIndex(bottom)))
            {
                --bottom;
            }

            auto savedPen = painter.pen();
            painter.setPen(gridPen);

            // row
            for (auto visualIndex = top; visualIndex <= bottom; ++visualIndex)
            {
                auto row = verticalHeader->logicalIndex(visualIndex);
                if (verticalHeader->isSectionHidden(row))
                    continue;

                auto rowY = rowViewportPosition(row) + offset.y();
                auto rowh = rowHeight(row) - gridSize;
                painter.drawLine(dirtyArea.left(), rowY + rowh, dirtyArea.right(), rowY + rowh);
            }

            // column
            for (auto h = left; h <= right; ++h)
            {
                auto col = horizontalHeader->logicalIndex(h);

                if (horizontalHeader->isSectionHidden(col))
                    continue;

                auto colp = columnViewportPosition(col) + offset.x();
                colp += columnWidth(col) - gridSize;
                painter.drawLine(colp, dirtyArea.top(), colp, dirtyArea.bottom());
            }

            painter.setPen(savedPen);
        }
    }
}

void LogViewTable::updateTreeView()
{
    verticalHeader()->reset();
    horizontalHeader()->reset();
    viewport()->update();
}

void LogViewTable::drawCell(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    bool bSelectedCell = false;
    auto selectionModelX = qobject_cast<const LogViewSelectionModel*>(selectionModel());
    if (selectionModelX)
        bSelectedCell = selectionModelX->IsSelectedEx(index.column(), index.row());

    auto proxy = qobject_cast<const LogViewModel*>(model());
    if (!proxy)
        return;

    auto savedColor = painter->pen().color();
    auto penColor = option.palette.color(QPalette::Normal, bSelectedCell ? QPalette::HighlightedText : QPalette::Text);

    auto customColor = proxy->data(index, Qt::ForegroundRole);
    if (customColor.isValid())
        penColor = customColor.value<QColor>();

    painter->setPen(penColor);

    if (bSelectedCell)
        painter->fillRect(option.rect, option.palette.brush(QPalette::Normal, QPalette::Highlight));
    else if ((option.features & QStyleOptionViewItem::Alternate) == QStyleOptionViewItem::Alternate)
        painter->fillRect(option.rect, option.palette.brush(QPalette::Normal, QPalette::AlternateBase));

    //    auto rc = option.rect;
    //    painter->drawRoundedRect(rc.adjusted(0, 0, -1, -1), 2, 2);

    option.widget->style()->drawItemText(painter,
        option.rect,
        Qt::AlignLeft | Qt::AlignVCenter,
        option.palette,
        true,
        proxy->data(index, Qt::DisplayRole).toString());
    painter->setPen(savedColor);
}

void LogViewTable::onSelectionChange()
{
    DEBUGTRACE();
    viewport()->update();
}

void LogViewTable::onCopySelected()
{
    DEBUGTRACE();

    auto selsection = static_cast<LogViewSelectionModel*>(selectionModel());
    LogViewSelectionModel::LogViewSelectionModelItem itemFirst{};
    LogViewSelectionModel::LogViewSelectionModelItem itemSecond{};
    if (!selsection->GetSelectedEx(&itemFirst, &itemSecond))
        return;

    int32_t top = 0;
    int32_t left = 0;
    int32_t bottom = 0;
    int32_t right = 0;

    if (itemFirst.column < itemSecond.column)
    {
        left = itemFirst.column;
        right = itemSecond.column;
    }
    else
    {
        left = itemSecond.column;
        right = itemFirst.column;
    }

    if (itemFirst.row < itemSecond.row)
    {
        top = itemFirst.row;
        bottom = itemSecond.row;
    }
    else
    {
        top = itemSecond.row;
        bottom = itemFirst.row;
    }

    auto itemsCount = (bottom - top + 1) * (right - left + 1);
//    if (itemsCount < 1)
//        return;

    try
    {
        auto modelX = dynamic_cast<LogViewModel*>(model());
        QString stream;

        constexpr auto clipboardMaxLines = 500;
        int totRow = 0;

        for (auto rowIdx = top; rowIdx <= bottom; rowIdx++)
        {

            for (auto colIdx = left; colIdx <= right; colIdx++)
            {
                if (colIdx != left)
                    stream += "\t";
                stream += modelX->dataEx(rowIdx);
            }

#if defined(_WIN32) || defined(_WIN64)
            stream += "\r\n";
#else
            stream += "\n";
#endif

            if(++totRow >= clipboardMaxLines)
                break;

        }

        QString toolTipText;
        if(totRow < 1)
        {
            toolTipText = QString("<center><b>Nothing to copy to the clipboard.</b></center>");
        }
        else
        {
            if(itemsCount != totRow)
            {
                toolTipText = QString("<center style=\"color:red;background-color:white;\">"
                                      "<b>Warning! Only the first %1 lines are copied to the clipboard.</b>"
                                      "</center>").arg(totRow);
            }
            else
            {
                toolTipText = QString("<center><b>%1 lines copied to clipboard</b></center>").arg(totRow);
            }
        }


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

        auto clipboard = QApplication::clipboard();
        if (clipboard)
        {
            clipboard->setText(stream);

            QToolTip::showText(this->mapToGlobal(QPoint(xPos, yPos)), toolTipText, this);

        }

    }
    catch (std::exception  const&e)
    {
        QMessageBox::critical(this, QObject::tr("Copy"), QObject::tr(e.what()), QMessageBox::Ok);
    }
    catch (...)
    {
        QMessageBox::critical(this, QObject::tr("Copy"), QObject::tr("Unexpected exception"), QMessageBox::Ok);
    }

}

void LogViewTable::onUp()
{
    setFocus();
    qApp->postEvent(this, new QKeyEvent (QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier));
    qApp->postEvent(this, new QKeyEvent (QEvent::KeyRelease, Qt::Key_Home, Qt::NoModifier));
//    qDebug() << "onUp()";
}

void LogViewTable::onDown()
{
    setFocus();
    qApp->postEvent(this, new QKeyEvent (QEvent::KeyPress, Qt::Key_End, Qt::NoModifier));
    qApp->postEvent(this, new QKeyEvent (QEvent::KeyRelease, Qt::Key_End, Qt::NoModifier));
    if(hasFocus())
        emit newRowPresent(false);
//    qDebug() << "onDown()";
}

void LogViewTable::onScrollbarValueChanged(int32_t scrollbarPosition)
{

    //DEBUGTRACE();
    if (scrollbarPosition < 0)
        scrollbarPosition = 0;

    auto mod = static_cast<LogViewModel*>(model());
    mod->setScrollbarPosition(scrollbarPosition);

    auto selsection = static_cast<LogViewSelectionModel*>(selectionModel());

    if(scrollbarPosition <= 0)
    {
        selsection->UpdateLock(true);
        setCurrentIndex(mod->index(0, 0));
        selsection->UpdateLock(false);
    }
    else if (scrollbarPosition >= m_pVerticalScrollBar->maximum())
    {
        selsection->UpdateLock(true);
        setCurrentIndex(mod->index(std::max(0, mod->rowCount(QModelIndex()) - 1), 0));
        selsection->UpdateLock(false);
    }

    auto scrollBarAtEnd = (m_pVerticalScrollBar->value() >= m_pVerticalScrollBar->maximum());
    if(scrollBarAtEnd && hasFocus())
    {
        //qDebug() << "  focus " << hasFocus();
        emit newRowPresent(false);
    }
    //qDebug() << "  ** AUTOSCROLL  ENABLE " << scrollBarAtEnd ;

    updateTreeView();

}

void LogViewTable::onColumnWidthChanged(int colWidth)
{
    auto displArea = width() - verticalHeader()->width();
    colWidth = std::max<int>(displArea, colWidth);
    auto val = columnWidth(0);
    if(val == colWidth)
        return;
    setColumnWidth(0, colWidth);
}

void LogViewTable::onXrowCountChanged(QFont *fontVal)
{

    QRect cr = contentsRect();

    int sectionHeight = 0;
    if (verticalHeader()->count() > 0)
        sectionHeight = verticalHeader()->sectionSize(0);

    if (sectionHeight < 1)
        sectionHeight = verticalHeader()->defaultSectionSize();

    auto mod = static_cast<LogViewModel*>(model());

    int rowsInFile = 0;
    int maxScrollBar = 0;

    mod->controlGeometryUpdate(cr.width(), cr.height(), sectionHeight, fontVal, m_visibleRow, rowsInFile, maxScrollBar);

//    qDebug() << " Update width" << cr.width()
//             << " height " << cr.height()
//             << " sectionHeight " << sectionHeight
//             << " fontVal " << fontVal
//             << " viewRows " << viewRows
//             << " rowsInFile " << rowsInFile
//             << " maxScrollBar " << maxScrollBar;

    auto maxSctoll = m_pVerticalScrollBar->maximum();
    if(maxSctoll !=maxScrollBar)
        m_pVerticalScrollBar->setMaximum(maxScrollBar);

    onColumnWidthChanged(mod->columnWidth());


    if(m_lastRow !=  rowsInFile)
    {
        m_lastRow =  rowsInFile;
        emit newRowPresent(true);
    }

    if(isAutoscrollEnabled())
    {
        onDown();
    }

    updateTreeView();

    emit updateInfo(mod->headerData(0, Qt::Orientation::Horizontal, Qt::DisplayRole).toString());

}

bool LogViewTable::isAutoscrollEnabled()
{
    return m_autoscroll;
//    auto scrollBarAtEnd = (m_pVerticalScrollBar->value() + 1 >= m_pVerticalScrollBar->maximum());
//    auto availableLineForScroll = (currentIndex().isValid() && currentIndex().row() + 1 >= m_visibleRow);
//    //qDebug() << "  AUTOSCROLL  scrollBarAtEnd: " << scrollBarAtEnd << " availableLineForScroll: " << availableLineForScroll;
//    return availableLineForScroll && scrollBarAtEnd;
//    //return (m_pVerticalScrollBar->value() + 1 >= m_pVerticalScrollBar->maximum()) && (currentIndex().isValid() && currentIndex().row() + 1 >= m_visibleRow);
}
