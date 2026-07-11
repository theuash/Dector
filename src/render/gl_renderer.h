#pragma once
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <memory>

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLTexture;
class QImage;

class GLRenderer : protected QOpenGLFunctions {
public:
    GLRenderer();
    ~GLRenderer();

    void initialize();
    void render(const QImage& frame, const QMatrix4x4& transform, float opacity);
    void resize(int width, int height);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};