#include "gl_renderer.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QDebug>

struct GLRenderer::Impl {
    QOpenGLShaderProgram program;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLTexture* texture = nullptr;
    int viewportW = 0, viewportH = 0;
    QMatrix4x4 projection;
};

GLRenderer::GLRenderer() : d(std::make_unique<Impl>()) {}
GLRenderer::~GLRenderer() {
    if (d->texture) {
        d->texture->destroy();
        delete d->texture;
    }
}

void GLRenderer::initialize() {
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shader program
    const char* vsSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        uniform mat4 uProjection;
        uniform mat4 uTransform;
        out vec2 vTexCoord;
        void main() {
            gl_Position = uProjection * uTransform * vec4(aPos, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )";
    const char* fsSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        uniform sampler2D uTexture;
        uniform float uOpacity;
        out vec4 fragColor;
        void main() {
            vec4 color = texture(uTexture, vTexCoord);
            fragColor = vec4(color.rgb * color.a, color.a * uOpacity);
        }
    )";

    d->program.addShaderFromSourceCode(QOpenGLShader::Vertex, vsSrc);
    d->program.addShaderFromSourceCode(QOpenGLShader::Fragment, fsSrc);
    d->program.link();

    // Full-screen quad
    float vertices[] = {
        // pos      texCoord
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 0.0f,
    };
    d->vao.create();
    d->vao.bind();
    d->vbo.create();
    d->vbo.bind();
    d->vbo.allocate(vertices, sizeof(vertices));
    d->program.bind();
    d->program.enableAttributeArray(0);
    d->program.enableAttributeArray(1);
    d->program.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
    d->program.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
    d->vao.release();
    d->vbo.release();
    d->program.release();

    // Texture
    d->texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    d->texture->setMinificationFilter(QOpenGLTexture::Linear);
    d->texture->setMagnificationFilter(QOpenGLTexture::Linear);
    d->texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void GLRenderer::render(const QImage& frame, const QMatrix4x4& transform, float opacity) {
    if (frame.isNull()) return;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, d->viewportW, d->viewportH);

    // Update texture
    d->texture->setData(frame);

    d->program.bind();
    d->program.setUniformValue("uProjection", d->projection);
    d->program.setUniformValue("uTransform", transform);
    d->program.setUniformValue("uOpacity", opacity);
    d->texture->bind();
    d->vao.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    d->vao.release();
    d->texture->release();
    d->program.release();
}

void GLRenderer::resize(int width, int height) {
    d->viewportW = width;
    d->viewportH = height;
    d->projection.setToIdentity();
    d->projection.ortho(-1, 1, -1, 1, -1, 1);
}