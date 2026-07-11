#pragma once
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <memory>
class QImage;

// ponytail: OpenGL renderer for compositing frames, week 2 implementation

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    void initialize();
    void render(const QImage& frame, const QMatrix4x4& transform, float opacity);
    void resize(int width, int height);

private:
    int m_viewportW = 0;
    int m_viewportH = 0;
};
