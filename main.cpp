/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

//#define MEMORYCHK

#if defined(MEMORYCHK) && (defined(_WIN32) || defined(_WIN64))
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>

////#ifdef _DEBUG
////#   define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
////#else
////#   define DBG_NEW new
////#endif // ! _DEBUG

#endif // ! defined(_WIN32) || defined(_WIN64)

#include "fileselector.h"
#include "iconhelper.h"
#include "logviewwidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>

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

int main(int argc, char *argv[])
{

#if defined(MEMORYCHK)
    _CrtDumpMemoryLeaks();
#endif

    QString path;

    if (path.isEmpty() && argc == 2)
        path = QString::fromLatin1(argv[1]);

#if 0

#   if defined(_WIN32) || defined(_WIN64)

#   elif defined (Q_OS_ANDROID)

#   else
    //path = "/media/f/Data/temp1/my.log.3";
    //path = "/media/f/Data/temp1/Untitled 1.csv";
    //path = "/home/f/Documents/Untitled 1.csv";
#   endif

//#   define TESTSELECTOR

#endif

    Q_INIT_RESOURCE(resources);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setOrganizationName("chipmunk-sm");
    QCoreApplication::setApplicationName("ChipmunkLogView");
    QCoreApplication::setApplicationVersion("1.0.0");

//    const QStringList styles = QStyleFactory::keys();
//    for (QString s : styles)
//    {
//        qDebug() << s;
//    }
//    qDebug() << QApplication::style()->objectName();

    try
    {
        FileSelector fs(true, path, QString("log.txt"));
        if(path.isEmpty() || path.compare(fs.getPath(),
#if defined(_WIN32) || defined(_WIN64)
                                           Qt::CaseInsensitive
#else
                                           Qt::CaseSensitive
#endif
                                           ) != 0)
        {
            path = fs.exec();
        }
    }
    catch (...)
    {
        QMessageBox mbessage(QCoreApplication::applicationName(), QObject::tr("Failed on select file. Abort."),
                             QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
        mbessage.setWindowIcon(IconHelper::CreateIcon("LOG", "select", mbessage.font()));
        mbessage.exec();
        return 0;
    }

    if(path.isEmpty())
    {
        return 0;
    }

    try
    {

        LogViewWidget logwidget(QCoreApplication::applicationName(), path);
        logwidget.show();
        QApplication::exec();
    }
    catch (...)
    {
        QMessageBox mbessage(QCoreApplication::applicationName(), QObject::tr("Failed on select file. Abort."),
                             QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
        mbessage.setWindowIcon(IconHelper::CreateIcon("LOG", "select", mbessage.font()));
        mbessage.exec();
        return 0;
    }

    return 0;



#if defined(MEMORYCHK)
   _CrtDumpMemoryLeaks();
#endif

    return 0;
}
