/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#include "stylehelper.h"

StyleHelper::StyleHelper(int32_t smallIconSize)
{
    m_smallIconSize = smallIconSize;
}

StyleHelper::~StyleHelper()
{

}

int StyleHelper::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch ( metric )
    {
    case QStyle::PM_SmallIconSize:
        return m_smallIconSize;
    default:
        return QProxyStyle::pixelMetric( metric, option, widget );
    }
}
