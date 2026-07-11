#include "gl_renderer.h"
#include <QDebug>
// ponytail: OpenGL rendering implementation in week 2

GLRenderer::GLRenderer() = default;
GLRenderer::~GLRenderer() = default;
void GLRenderer::initialize() { qDebug() << "GLRenderer stub"; }
void GLRenderer::render(const QImage&, const QMatrix4x4&, float) {}
void GLRenderer::resize(int, int) {}
