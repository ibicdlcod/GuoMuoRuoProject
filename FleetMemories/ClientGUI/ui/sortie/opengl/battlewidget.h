// Copyright (C) 2016 The Qt Company Ltd.
/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: BSD-3-Clause */

#ifndef BATTLEWIDGET_H
#define BATTLEWIDGET_H

#include "geometryengine.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QBasicTimer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

class GeometryEngine;

class BattleWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    using QOpenGLWidget::QOpenGLWidget;
    ~BattleWidget();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void timerEvent(QTimerEvent *e) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void initShaders();

private:
    QBasicTimer timer;
    QOpenGLShaderProgram program;
    GeometryEngine *geometries = nullptr;

    QMatrix4x4 projection;

    QVector2D mousePressPosition;
    QVector3D rotationAxis;
    qreal angularSpeed = 0;
    QQuaternion rotation;
};

#endif // BATTLEWIDGET_H
