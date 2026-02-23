/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAPVIEWWIDGET_H
#define MAPVIEWWIDGET_H

#include <QWidget>
#include <QBoxLayout>
#include <QStackedLayout>

class MapViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapViewWidget(QList<QWidget *> subWidgets,
                           float width,
                           float height,
                           QWidget *parent = nullptr);
    void resizeEvent(QResizeEvent *event);

    void setCurrentWidget(QWidget *widget);

private:
    QBoxLayout *layout;
    QStackedLayout *innerLayout;
    float arWidth; // aspect ratio width
    float arHeight; // aspect ratio height
};

#endif // MAPVIEWWIDGET_H
