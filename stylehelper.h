/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QProxyStyle>

class StyleHelper: public QProxyStyle
{
    Q_OBJECT
public:
    StyleHelper(int32_t smallIconSize);
    virtual ~StyleHelper() override;
    virtual int pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr ) const override;
    int32_t m_smallIconSize = 16;
};
#endif // STYLEHELPER_H
