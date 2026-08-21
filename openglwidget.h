#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include "detector.h"
#include <QImage>
#include <QVideoFrame>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QList>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

public slots:
    void clearFrame();
    void onvideoFrameChanged(const QVideoFrame &frame);
    void ondetectionResultReady(const QList<Detection> &results);

public:
    QImage grabCurrentFrame();

private:
    void cleanup();

    void initFrameShaders();
    void initFrameBuffers();
    void frameRenderer();
    void updateFrameVertices();

    void initBoxShaders();
    void initBoxBuffers();
    void boxesRenderer();
    void updateBoxVertices();

    void drawLabels();

    QImage frame_;
    QList<Detection> detections_;
    QOpenGLVertexArrayObject frameVao_;
    QOpenGLBuffer frameVbo_;
    QOpenGLBuffer frameEbo_{QOpenGLBuffer::IndexBuffer};
    QOpenGLShaderProgram *frameProgram_;
    QOpenGLShader *frameVertexShader_;
    QOpenGLShader *frameFragmentShader_;
    QOpenGLTexture *frameTexture_;

    QOpenGLVertexArrayObject boxVao_;
    QOpenGLBuffer boxVbo_;
    QOpenGLShaderProgram *boxProgram_;
    QOpenGLShader *boxVertexShader_;
    QOpenGLShader *boxFragmentShader_;
    std::vector<float> boxVertices_;

    float frameAspectRatio_ = 1.0f;
    int frameWidth_ = 0;
    int frameHeight_ = 0;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
};

#endif // OPENGLWIDGET_H
