#include "openglwidget.h"

#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , frameProgram_(nullptr), frameVertexShader_(nullptr), frameFragmentShader_(nullptr)
    , frameTexture_(nullptr)
    , boxProgram_(nullptr), boxVertexShader_(nullptr), boxFragmentShader_(nullptr)
{
}

OpenGLWidget::~OpenGLWidget()
{
    cleanup();
}

void OpenGLWidget::clearFrame()
{
    frame_ = QImage();
    update();
}

void OpenGLWidget::onvideoFrameChanged(const QVideoFrame &frame)
{
    if (!frame.isValid())
        return;
    frame_ = frame.toImage();

    frameWidth_ = frame_.width();
    frameHeight_ = frame_.height();
    if (frameHeight_ > 0) {
        frameAspectRatio_ = static_cast<float>(frameWidth_) / frameHeight_;
    }

    updateFrameVertices();

    update();
}

void OpenGLWidget::ondetectionResultReady(const QList<Detection> &results)
{
    detections_ = results;
    update();
}

QImage OpenGLWidget::grabCurrentFrame()
{
    return grabFramebuffer();
}

void OpenGLWidget::cleanup()
{
    makeCurrent();

    if (frameTexture_) {
        delete frameTexture_;
        frameTexture_ = nullptr;
    }

    if (frameVertexShader_) {
        delete frameVertexShader_;
        frameVertexShader_ = nullptr;
    }

    if (frameFragmentShader_) {
        delete frameFragmentShader_;
        frameFragmentShader_ = nullptr;
    }

    if (frameProgram_) {
        delete frameProgram_;
        frameProgram_ = nullptr;
    }

    frameVbo_.destroy();
    frameEbo_.destroy();
    frameVao_.destroy();

    if (boxVertexShader_) {
        delete boxVertexShader_;
        boxVertexShader_ = nullptr;
    }

    if (boxFragmentShader_) {
        delete boxFragmentShader_;
        boxFragmentShader_ = nullptr;
    }

    if (boxProgram_) {
        delete boxProgram_;
        boxProgram_ = nullptr;
    }

    boxVbo_.destroy();
    boxVao_.destroy();

    doneCurrent();

    disconnect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &OpenGLWidget::cleanup);
}

void OpenGLWidget::initFrameShaders()
{
    const char *vertSrc = R"(
        #version 450 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vTexCoord   = aTexCoord;
        }
    )";
    frameVertexShader_ = new QOpenGLShader(QOpenGLShader::Vertex, this);
    if (!frameVertexShader_->compileSourceCode(vertSrc))
        qWarning() << "[vert]" << frameVertexShader_->log();

    const char *fragSrc = R"(
        #version 450 core
        in  vec2      vTexCoord;
        out vec4      fragColor;
        uniform sampler2D uTex;
        void main() {
            fragColor = texture(uTex, vTexCoord);
        }
    )";
    frameFragmentShader_ = new QOpenGLShader(QOpenGLShader::Fragment, this);
    if (!frameFragmentShader_->compileSourceCode(fragSrc))
        qWarning() << "[frag]" << frameFragmentShader_->log();

    frameProgram_ = new QOpenGLShaderProgram(this);
    frameProgram_->addShader(frameVertexShader_);
    frameProgram_->addShader(frameFragmentShader_);
    if (!frameProgram_->link())
        qWarning() << "[link]" << frameProgram_->log();
}

void OpenGLWidget::initBoxShaders()
{
    const char *vertSrc = R"(
        #version 450 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec4 aColor;
        out vec4 vColor;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vColor = aColor;
        }
    )";
    boxVertexShader_ = new QOpenGLShader(QOpenGLShader::Vertex, this);
    if (!boxVertexShader_->compileSourceCode(vertSrc))
        qWarning() << "[box vert]" << boxVertexShader_->log();

    const char *fragSrc = R"(
        #version 450 core
        in vec4 vColor;
        out vec4 fragColor;
        void main() {
            fragColor = vColor;
        }
    )";
    boxFragmentShader_ = new QOpenGLShader(QOpenGLShader::Fragment, this);
    if (!boxFragmentShader_->compileSourceCode(fragSrc))
        qWarning() << "[box frag]" << boxFragmentShader_->log();

    boxProgram_ = new QOpenGLShaderProgram(this);
    boxProgram_->addShader(boxVertexShader_);
    boxProgram_->addShader(boxFragmentShader_);
    if (!boxProgram_->link())
        qWarning() << "[box link]" << boxProgram_->log();
}

void OpenGLWidget::initFrameBuffers()
{
    static const GLfloat verts[] = {
        // x      y      s     t
        -1.0f,  1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,
    };
    static const GLuint idx[] = { 0, 1, 2,  0, 2, 3 };

    frameVao_.create();
    frameVao_.bind();

    frameVbo_.create();
    frameVbo_.bind();
    frameVbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    frameVbo_.allocate(verts, sizeof(verts));

    frameEbo_.create();
    frameEbo_.bind();
    frameEbo_.allocate(idx, sizeof(idx));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat),
                          reinterpret_cast<void *>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat),
                          reinterpret_cast<void *>(2 * sizeof(GLfloat)));

    frameVao_.release();
}

void OpenGLWidget::updateFrameVertices()
{
    if (frameWidth_ == 0 || frameHeight_ == 0)
        return;

    int widgetWidth = width();
    int widgetHeight = height();
    if (widgetHeight == 0) return;

    float widgetAspectRatio = static_cast<float>(widgetWidth) / widgetHeight;

    float xScale = 1.0f;
    float yScale = 1.0f;

    if (frameAspectRatio_ > widgetAspectRatio) {
        yScale = widgetAspectRatio / frameAspectRatio_;
    } else {
        xScale = frameAspectRatio_ / widgetAspectRatio;
    }

    static const GLfloat verts[] = {
        // x      y      s     t
        -1.0f,  1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,
    };

    GLfloat adaptiveVerts[16];
    for (int i = 0; i < 16; i += 4) {
        adaptiveVerts[i] = verts[i] * xScale;     // x
        adaptiveVerts[i+1] = verts[i+1] * yScale; // y
        adaptiveVerts[i+2] = verts[i+2];           // s (texture coordinate)
        adaptiveVerts[i+3] = verts[i+3];           // t (texture coordinate)
    }

    frameVbo_.bind();
    frameVbo_.write(0, adaptiveVerts, sizeof(adaptiveVerts));
    frameVbo_.release();
}

void OpenGLWidget::initBoxBuffers()
{
    boxVao_.create();
    boxVao_.bind();

    boxVbo_.create();
    boxVbo_.bind();
    boxVbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat),
                          reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat),
                          reinterpret_cast<void*>(2 * sizeof(GLfloat)));

    boxVao_.release();
}

void OpenGLWidget::updateBoxVertices()
{
    if (frame_.isNull())
        return;

    float widgetAspect = static_cast<float>(width()) / height();
    float frameAspect = frameAspectRatio_;

    float xScale = 1.0f, yScale = 1.0f;
    if (frameAspect > widgetAspect) {
        yScale = widgetAspect / frameAspect;
    } else {
        xScale = frameAspect / widgetAspect;
    }

    boxVertices_.clear();

    for (const Detection &det : detections_) {
        int hue = (det.classId * 37) % 360;
        QColor color = QColor::fromHsv(hue, 200, 230);
        float r = color.redF();
        float g = color.greenF();
        float b = color.blueF();
        float a = 1.0f;

        float x1 = (1.0f - det.x1 / frameWidth_ * 2.0f) * xScale;
        float y1 = (1.0f - det.y1 / frameHeight_ * 2.0f) * yScale;
        float x2 = (1.0f - det.x2 / frameWidth_ * 2.0f) * xScale;
        float y2 = (1.0f - det.y2 / frameHeight_ * 2.0f) * yScale;

        boxVertices_.insert(boxVertices_.end(), {x1, y1, r, g, b, a});
        boxVertices_.insert(boxVertices_.end(), {x2, y1, r, g, b, a});

        boxVertices_.insert(boxVertices_.end(), {x2, y1, r, g, b, a});
        boxVertices_.insert(boxVertices_.end(), {x2, y2, r, g, b, a});

        boxVertices_.insert(boxVertices_.end(), {x2, y2, r, g, b, a});
        boxVertices_.insert(boxVertices_.end(), {x1, y2, r, g, b, a});

        boxVertices_.insert(boxVertices_.end(), {x1, y2, r, g, b, a});
        boxVertices_.insert(boxVertices_.end(), {x1, y1, r, g, b, a});
    }

    boxVbo_.bind();
    boxVbo_.allocate(boxVertices_.data(),
                     static_cast<int>(boxVertices_.size() * sizeof(GLfloat)));
    boxVbo_.release();
}

void OpenGLWidget::boxesRenderer()
{
    if (frame_.isNull())
        return;

    updateBoxVertices();

    boxProgram_->bind();
    boxVao_.bind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLineWidth(2.0f);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(boxVertices_.size() / 6));

    glDisable(GL_BLEND);

    boxVao_.release();
    boxProgram_->release();
}

void OpenGLWidget::drawLabels()
{
    if (frame_.isNull())
        return;

    float xScale = 1.0f, yScale = 1.0f;
    float widgetAspect = static_cast<float>(width()) / height();
    float frameAspect = frameAspectRatio_;

    if (frameAspect > widgetAspect) {
        yScale = widgetAspect / frameAspect;
    } else {
        xScale = frameAspect / widgetAspect;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Arial", 10, QFont::Bold));

    for (const Detection &det : detections_) {
        int hue = (det.classId * 37) % 360;
        QColor color = QColor::fromHsv(hue, 200, 230);

        float x1 = (1.0f - det.x1 / frameWidth_ * 2.0f) * xScale;
        float y1 = (1.0f - det.y1 / frameHeight_ * 2.0f) * yScale;
        float x2 = (1.0f - det.x2 / frameWidth_ * 2.0f) * xScale;
        float y2 = (1.0f - det.y2 / frameHeight_ * 2.0f) * yScale;

        int wx1 = static_cast<int>((x1 + 1.0f) / 2.0f * width());
        int wy1 = static_cast<int>((1.0f - y1) / 2.0f * height());
        int wx2 = static_cast<int>((x2 + 1.0f) / 2.0f * width());
        int wy2 = static_cast<int>((1.0f - y2) / 2.0f * height());

        QString idStr = (det.trackId >= 0) ? QString::number(det.trackId) : QStringLiteral("?");

        QString label = QString("ID %1 %2: %3%")
                            .arg(idStr)
                            .arg(Detector::className(det.classId))
                            .arg(det.score * 100, 0, 'f', 1);

        QFontMetrics fm(painter.font());
        QRect textRect = fm.boundingRect(label);
        textRect.adjust(-2, -2, 2, 2);
        textRect.moveBottomLeft(QPoint(wx2, wy1 - 2));

        painter.fillRect(textRect, QColor(0, 0, 0, 100));

        painter.setPen(color);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }

    painter.end();
}

void OpenGLWidget::frameRenderer()
{
    if (frame_.isNull()) {
        if (frameTexture_) {
            delete frameTexture_;
            frameTexture_ = nullptr;
        }
        return;
    }

    if (!frameTexture_ ||
        frameTexture_->width()  != frame_.width() ||
        frameTexture_->height() != frame_.height())
    {
        delete frameTexture_;
        frameTexture_ = new QOpenGLTexture(frame_);
        frameTexture_->setMinificationFilter(QOpenGLTexture::Linear);
        frameTexture_->setMagnificationFilter(QOpenGLTexture::Linear);
        frameTexture_->setWrapMode(QOpenGLTexture::ClampToEdge);
    } else {
        frameTexture_->bind();
        frameTexture_->setData(QOpenGLTexture::RGBA,
                               QOpenGLTexture::UInt8,
                               frame_.constBits());
        frameTexture_->release();
    }

    frameProgram_->bind();
    frameTexture_->bind(0);
    frameProgram_->setUniformValue("uTex", 0);

    frameVao_.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    frameVao_.release();

    frameTexture_->release();
    frameProgram_->release();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    initFrameShaders();
    initFrameBuffers();
    initBoxShaders();
    initBoxBuffers();

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &OpenGLWidget::cleanup);
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    updateFrameVertices(); // Recalculate vertices when widget is resized
    updateBoxVertices();   // Recalculate box vertices when widget is resized
}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    frameRenderer();
    boxesRenderer();
    drawLabels();
}
