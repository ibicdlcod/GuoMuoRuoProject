// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "geometryengine.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"

#include <QVector2D>
#include <QVector3D>
#include <iostream>

struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
};

void dbgModel(tinygltf::Model &model) {
    for (auto &mesh : model.meshes) {
        std::cout << "mesh : " << mesh.name << std::endl;
        for (auto &primitive : mesh.primitives) {
            const tinygltf::Accessor &indexAccessor =
                model.accessors[primitive.indices];

            std::cout << "indexaccessor: count " << indexAccessor.count << ", type "
                      << indexAccessor.componentType << std::endl;

            tinygltf::Material &mat = model.materials[primitive.material];
            for (auto &mats : mat.values) {
                std::cout << "mat : " << mats.first.c_str() << std::endl;
            }

            for (auto &image : model.images) {
                std::cout << "image name : " << image.uri << std::endl;
                std::cout << "  size : " << image.image.size() << std::endl;
                std::cout << "  w/h : " << image.width << "/" << image.height
                          << std::endl;
            }

            std::cout << "indices : " << primitive.indices << std::endl;
            std::cout << "mode     : "
                      << "(" << primitive.mode << ")" << std::endl;

            for (auto &attrib : primitive.attributes) {
                std::cout << "attribute : " << attrib.first.c_str() << std::endl;
            }
        }
    }
}
//! [0]
GeometryEngine::GeometryEngine()
    : indexBuf(QOpenGLBuffer::IndexBuffer)
{
    initializeOpenGLFunctions();

    // Generate 2 VBOs
    arrayBuf.create();
    indexBuf.create();

    // Initializes cube geometry and transfers it to VBOs
    loadGLB("new.gltf");
}

GeometryEngine::~GeometryEngine()
{
    arrayBuf.destroy();
    indexBuf.destroy();
}
//! [0]
void GeometryEngine::loadGLB(const QString &path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    if (!loader.LoadASCIIFromFile(&model, &err, &warn, path.toStdString())) {
        //% "Load model %1 failed!"
        qWarning() << qtTrId("load-model-failed").arg(path);
        return;
    }
    m_meshTransforms.resize(model.meshes.size());
    const tinygltf::Scene &scene = model.scenes[model.defaultScene];
    for (int nodeIdx : scene.nodes) {
        calculateNodeTransforms(model, nodeIdx, QMatrix4x4()); // Identity parent
    }
    // Iterate through all meshes in the file
    int i = 0;
    for (const auto &mesh : model.meshes) {
        // Iterate through all primitives (sub-parts) of the mesh
        for (const auto &primitive : mesh.primitives) {
            PrimitiveBuffers *part = new PrimitiveBuffers();
            if (primitive.attributes.count("TEXCOORD_0")) {
                const auto &uvAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto &uvView = model.bufferViews[uvAcc.bufferView];
                const auto &uvBuf = model.buffers[uvView.buffer];

                // Create a separate VBO for UVs or interleave them.
                // Here, we create a second VBO for simplicity:
                part->uvVbo.create();
                part->uvVbo.bind();
                part->uvVbo.allocate(&uvBuf.data[uvView.byteOffset + uvAcc.byteOffset],
                                     uvAcc.count * 2 * sizeof(float));
            }
            if (primitive.attributes.count("NORMAL")) {
                const auto &normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const auto &normView = model.bufferViews[normAcc.bufferView];
                const auto &normBuf = model.buffers[normView.buffer];

                part->normalVbo.create();
                part->normalVbo.bind();
                part->normalVbo.allocate(&normBuf.data[normView.byteOffset + normAcc.byteOffset],
                                         normAcc.count * 3 * sizeof(float)); // 3 floats: x, y, z
            }

            // Associate the texture with this primitive
            part->texture = loadGLBTexture(model, primitive.material);
            part->globalTransform = m_meshTransforms[i];

            // --- 1. Position Buffer ---
            const auto &posAcc = model.accessors[primitive.attributes.at("POSITION")];
            const auto &posView = model.bufferViews[posAcc.bufferView];
            const auto &posBuf = model.buffers[posView.buffer];

            part->vbo.create();
            part->vbo.bind();
            part->vbo.allocate(&posBuf.data[posView.byteOffset + posAcc.byteOffset],
                               posAcc.count * 3 * sizeof(float));

            // --- 2. Index Buffer ---
            const auto &idxAcc = model.accessors[primitive.indices];
            const auto &idxView = model.bufferViews[idxAcc.bufferView];
            const auto &idxBuf = model.buffers[idxView.buffer];

            part->indexCount = idxAcc.count;
            part->ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
            part->ebo.create();
            part->ebo.bind();
            part->ebo.allocate(&idxBuf.data[idxView.byteOffset + idxAcc.byteOffset],
                               idxAcc.count * sizeof(unsigned short));

            part->mode = primitive.mode; // Usually GL_TRIANGLES (4)
            m_primitives.append(part);
        }
        ++i;
    }
}

//! [2]
void GeometryEngine::drawGeometry(QOpenGLShaderProgram *program, const QMatrix4x4 &viewProjection) {
    arrayBuf.bind();
    indexBuf.bind();
    // Locate the attribute and uniform in the shader
    int vertexLocation = program->attributeLocation("a_position");
    int mvpLocation = program->uniformLocation("mvp_matrix");

    // Loop through every mesh primitive found in the GLB
    for (PrimitiveBuffers *part : m_primitives) {

        // Combine the Camera/Lens matrix with the specific Node's position
        // MVP = (Projection * View) * NodeTransform
        QMatrix4x4 finalMvp = viewProjection * part->globalTransform;

        // Upload the specific matrix for this part to the shader
        program->setUniformValue(mvpLocation, finalMvp);

        // Calculate Normal Matrix: inverse transpose of the Model matrix
        // (Note: In simple cases without non-uniform scaling, you can use the model matrix itself)
        program->setUniformValue("normal_matrix", part->globalTransform.normalMatrix());

        // Pass a static light direction (e.g., coming from the top-right)
        program->setUniformValue("u_lightDirection", QVector3D(1.0f, 1.0f, 0.5f));

        // Bind Normals
        if (part->normalVbo.isCreated()) {
            part->normalVbo.bind();
            int normalLoc = program->attributeLocation("a_normal");
            program->enableAttributeArray(normalLoc);
            program->setAttributeBuffer(normalLoc, GL_FLOAT, 0, 3);
        }

        // Bind Texture
        if (part->texture) {
            part->texture->bind(0); // Bind to texture unit 0
            program->setUniformValue("u_texture", 0);
        }

        // Bind this part's specific vertex and index buffers
        part->vbo.bind();
        program->enableAttributeArray(vertexLocation);
        program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3);

        // Bind UVs
        if (part->uvVbo.isCreated()) {
            part->uvVbo.bind();
            program->enableAttributeArray("a_texcoord");
            program->setAttributeBuffer("a_texcoord", GL_FLOAT, 0, 2);
        }

        part->ebo.bind();

        // Draw the triangles using the index count stored during loading
        glDrawElements(part->mode, part->indexCount, GL_UNSIGNED_SHORT, nullptr);

        // Release for the next primitive
        part->vbo.release();
        part->uvVbo.release();
        part->normalVbo.release();
        part->ebo.release();
    }
}
//! [2]
void GeometryEngine::calculateNodeTransforms(const tinygltf::Model &model,
                                             int nodeIdx,
                                             const QMatrix4x4 &parentMatrix)
{
    const tinygltf::Node &node = model.nodes[nodeIdx];
    QMatrix4x4 localMatrix;

    // 1. Extract Local Transform (glTF nodes use TRS or a Matrix)
    if (node.matrix.size() == 16) {
        // Use the 4x4 matrix provided in the file
        float mat[16];
        for(int i=0; i<16; ++i) mat[i] = static_cast<float>(node.matrix[i]);
        localMatrix = QMatrix4x4(mat);
    } else {
        // Build from Translation, Rotation, Scale (TRS)
        if (node.translation.size() == 3)
            localMatrix.translate(node.translation[0], node.translation[1], node.translation[2]);
        if (node.rotation.size() == 4) {
            QQuaternion q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            localMatrix.rotate(q);
        }
        if (node.scale.size() == 3)
            localMatrix.scale(node.scale[0], node.scale[1], node.scale[2]);
    }

    QMatrix4x4 worldMatrix = parentMatrix * localMatrix;

    // 2. If this node has a mesh, store its world matrix
    if (node.mesh > -1) {
        m_meshTransforms[node.mesh] = worldMatrix;
    }

    // 3. Recurse through children
    for (int childIdx : node.children) {
        calculateNodeTransforms(model, childIdx, worldMatrix);
    }
}

QOpenGLTexture* GeometryEngine::loadGLBTexture(const tinygltf::Model &model, int meshMaterialIdx) {
    if (meshMaterialIdx < 0) return nullptr;

    const tinygltf::Material &mat = model.materials[meshMaterialIdx];
    // Find the base color texture index
    int texIdx = mat.pbrMetallicRoughness.baseColorTexture.index;
    if (texIdx < 0) return nullptr;

    const tinygltf::Texture &tex = model.textures[texIdx];
    const tinygltf::Image &image = model.images[tex.source];

    // Convert raw tinygltf image bytes to QImage
    QImage qimg(image.image.data(), image.width, image.height, QImage::Format_RGBA8888);

    QOpenGLTexture *glTexture = new QOpenGLTexture(qimg.mirrored()); // OpenGL expects flipped Y
    glTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    glTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    return glTexture;
}
