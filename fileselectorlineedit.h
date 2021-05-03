/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTORLINEEDIT_H
#define FILESELECTORLINEEDIT_H

#include <QLineEdit>

class FileSelectorLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    FileSelectorLineEdit(QWidget *parent);
    ~FileSelectorLineEdit() override;
signals:
    void OnUpdateButton();

private slots:
    void updateButtons(const QString &val);

private:
    void resizeEvent(QResizeEvent *) override;
    QToolButton *m_button = nullptr;

};

#endif // FILESELECTORLINEEDIT_H
