#ifndef DETECTOR_H
#define DETECTOR_H

#include <QObject>
#include <QVideoFrame>
#include <QImage>
#include <QTimer>
#include <QList>

#include <onnxruntime_cxx_api.h>
#include <memory>
#include <vector>

#include "detection.h"
#include "tracker.h"

class Detector : public QObject
{
    Q_OBJECT
public:
    explicit Detector(QObject *parent = nullptr);
    ~Detector();

    bool isRunning() const { return timer_ && timer_->isActive(); }
    void detect();

    static QString className(int classId);

public slots:
    void init();
    void onvideoFrameChanged(const QVideoFrame &frame);

    void startDetection();
    void stopDetection();

private:
    bool initOrtSession();
    void preprocess(const QImage &img, std::vector<float> &tensor,
                    float &scale, int &padX, int &padY);
    void postprocess(const float *outData, int numDetections,
                     float scale, int padX, int padY);

    QImage frame_;
    QTimer *timer_ = nullptr;

    Ort::Env env_;
    Ort::SessionOptions sessionOptions_;
    std::unique_ptr<Ort::Session> session_;
    Ort::MemoryInfo memoryInfo_{nullptr};
    Ort::RunOptions runOptions_;

    bool modelReady_ = false;
    QList<Detection> detections_;

    Tracker tracker_;

signals:
    void inferenceStateChanged(bool isRunning);
    void detectionResultReady(const QList<Detection> &results);
};

#endif // DETECTOR_H
