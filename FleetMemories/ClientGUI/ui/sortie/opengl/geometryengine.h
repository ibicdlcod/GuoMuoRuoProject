// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include "tiny_gltf.h"

struct PrimitiveBuffers {
    QOpenGLBuffer vbo;
    QOpenGLBuffer uvVbo;      // Texture coordinates (u, v) <-- ADD THIS
    QOpenGLBuffer normalVbo; // <-- Add this for Normals
    QOpenGLBuffer ebo;
    int indexCount;
    int mode; // e.g., GL_TRIANGLES
    QMatrix4x4 globalTransform;
    QOpenGLTexture *texture = nullptr;

    // Destructor to clean up GPU resources
    ~PrimitiveBuffers() {
        if (vbo.isCreated()) vbo.destroy();
        if (uvVbo.isCreated()) uvVbo.destroy();
        if (ebo.isCreated()) ebo.destroy();
        if (texture) delete texture;
    }
};

class GeometryEngine : protected QOpenGLFunctions
{
public:
    GeometryEngine();
    virtual ~GeometryEngine();

    void loadGLB(const QString &path);
    void drawGeometry(QOpenGLShaderProgram *program, const QMatrix4x4 &viewProjection);
    void calculateNodeTransforms(const tinygltf::Model &model,
                                 int nodeIdx,
                                 const QMatrix4x4 &parentMatrix);
    QOpenGLTexture* loadGLBTexture(const tinygltf::Model &model, int meshMaterialIdx);

private:
    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;

    int m_indexCount = 0;
    QList<PrimitiveBuffers*> m_primitives;
    QList<QMatrix4x4> m_meshTransforms;
};

#endif // GEOMETRYENGINE_H
