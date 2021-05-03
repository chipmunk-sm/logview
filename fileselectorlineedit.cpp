/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "fileselectorlineedit.h"

#include <QToolButton>
#include <QStyle>
#include <QCompleter>
#include <QFileSystemModel>
//#include <QFileSystemModel>
#include <QDirModel>

FileSelectorLineEdit::FileSelectorLineEdit(QWidget *parent)
    : QLineEdit(parent)
{

    auto completer = new  QCompleter(this);
    //auto fsModel = new QDirModel(completer);
    QFileSystemModel *fsModel = new QFileSystemModel(completer);
    fsModel->setRootPath(QString());
    fsModel->setFilter(QDir::AllDirs | QDir::Drives | QDir::NoDotAndDotDot | QDir::Files);
    completer->setModel(fsModel);
    completer->setMaxVisibleItems(15);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

#ifdef Q_OS_WIN
    completer->setCaseSensitivity(Qt::CaseInsensitive);
#else
    completer->setCaseSensitivity(Qt::CaseSensitive);
#endif
    completer->setWrapAround(true);

//#ifdef Q_OS_WIN
//   fsModel->setRootPath("c:/");
//#else
//    fsModel->setRootPath("/");
//#endif

    setCompleter(completer);

    m_button = new QToolButton(this);
    m_button->setStyleSheet(QLatin1String("QToolButton { border: none; padding: 0px; }"));
    m_button->setCursor(Qt::ArrowCursor);
    m_button->setText("X");
    m_button->hide();

    connect(m_button, &QToolButton::clicked, this, &QLineEdit::clear);
    connect(this, &QLineEdit::textChanged, this, &FileSelectorLineEdit::updateButtons);

}

FileSelectorLineEdit::~FileSelectorLineEdit()
{
    delete m_button;
}

void FileSelectorLineEdit::resizeEvent(QResizeEvent *)
{
    auto sz = m_button->sizeHint();
    m_button->move(rect().right() - style()->pixelMetric(QStyle::PM_DefaultFrameWidth) - sz.width(), (rect().bottom() + 1 - sz.height())/2);
}

void FileSelectorLineEdit::updateButtons(const QString& val)
{
    m_button->setVisible(!val.isEmpty());
}
