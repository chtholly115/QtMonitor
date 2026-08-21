#include "detector.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMetaType>

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr int kInputSize = 640;
constexpr float kConfThreshold = 0.25f;
}

Detector::Detector(QObject *parent)
    : QObject{parent}
    , env_(ORT_LOGGING_LEVEL_WARNING, "QtMonitorDetector")
    , memoryInfo_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
    qRegisterMetaType<QList<Detection>>("QList<Detection>");

    sessionOptions_.SetIntraOpNumThreads(4);
    sessionOptions_.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
}

Detector::~Detector()
{
    if (timer_) {
        timer_->stop();
        timer_->deleteLater();
        timer_ = nullptr;
    }
}

void Detector::detect()
{
    if (!modelReady_ || frame_.isNull())
        return;

    try {
        std::vector<float> inputTensorData;
        float scale = 1.0f;
        int padX = 0, padY = 0;
        preprocess(frame_, inputTensorData, scale, padX, padY);

        std::array<int64_t, 4> inputShape{1, 3, kInputSize, kInputSize};
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo_, inputTensorData.data(), inputTensorData.size(),
            inputShape.data(), inputShape.size());

        const char *inputNames[] = {"images"};
        const char *outputNames[] = {"output0"};
        auto outputs = session_->Run(runOptions_, inputNames, &inputTensor, 1, outputNames, 1);

        if (!outputs.empty()) {
            const float *outData = outputs[0].GetTensorData<float>();
            const auto outShape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
            if (outShape.size() == 3 && outShape[1] == 300 && outShape[2] == 6) {
                postprocess(outData, static_cast<int>(outShape[1]), scale, padX, padY);

                // 使用 IOU 跟踪器为每个检测框分配 trackId
                detections_ = tracker_.update(detections_);

                if (!detections_.isEmpty()) {
                    qDebug() << "Detected" << detections_.size() << "objects, first:"
                             << className(detections_.first().classId)
                             << "score" << detections_.first().score
                             << "trackId" << detections_.first().trackId;
                }

                emit detectionResultReady(detections_);
            } else {
                qWarning() << "Detector: 意外的输出形状";
            }
        }
    } catch (const Ort::Exception &e) {
        qWarning() << "Detector: 推理异常:" << e.what();
    }
}

QString Detector::className(int classId)
{
    static const char *const names[] = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train",
        "truck", "boat", "traffic light", "fire hydrant", "stop sign",
        "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep",
        "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
        "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard",
        "sports ball", "kite", "baseball bat", "baseball glove", "skateboard",
        "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork",
        "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
        "couch", "potted plant", "bed", "dining table", "toilet", "tv",
        "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave",
        "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
        "scissors", "teddy bear", "hair drier", "toothbrush"
    };
    constexpr int count = static_cast<int>(sizeof(names) / sizeof(names[0]));
    if (classId >= 0 && classId < count)
        return QString::fromLatin1(names[classId]);
    return QStringLiteral("class-%1").arg(classId);
}

void Detector::init()
{
    initOrtSession();

    if (!timer_) {
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, &Detector::detect);
    }
}

void Detector::onvideoFrameChanged(const QVideoFrame &frame)
{
    if (!frame.isValid())
        return;
    frame_ = frame.toImage();
}

void Detector::startDetection()
{
    if (!timer_) {
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, &Detector::detect);
    }

    tracker_.reset();   // 重新开始跟踪，清空旧轨迹
    timer_->start(50);
    emit inferenceStateChanged(true);
}

void Detector::stopDetection()
{
    if (timer_ && timer_->isActive()) {
        timer_->stop();
        tracker_.reset();   // 停止后清空轨迹
        qDebug() << "Detection stopped";
        emit detectionResultReady({});
        emit inferenceStateChanged(false);
    } else {
        qDebug() << "Timer is not running";
    }
}

bool Detector::initOrtSession()
{
    QString modelPath;
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/yolo26n.onnx"),
        QStringLiteral("yolo26n.onnx"),
    };
    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate)) {
            modelPath = candidate;
            break;
        }
    }

    if (modelPath.isEmpty()) {
        qWarning() << "Detector: 找不到模型文件 yolo26n.onnx";
        modelReady_ = false;
        return false;
    }

    try {
        session_ = std::make_unique<Ort::Session>(env_, modelPath.toStdWString().c_str(), sessionOptions_);
        modelReady_ = true;
        qInfo() << "Detector: 已加载模型" << modelPath;
        return true;
    } catch (const Ort::Exception &e) {
        qWarning() << "Detector: 模型加载失败:" << e.what();
        modelReady_ = false;
        return false;
    }
}

void Detector::preprocess(const QImage &img, std::vector<float> &tensor,
                          float &scale, int &padX, int &padY)
{
    scale = std::min(static_cast<float>(kInputSize) / img.width(),
                     static_cast<float>(kInputSize) / img.height());
    const int newW = static_cast<int>(std::round(img.width() * scale));
    const int newH = static_cast<int>(std::round(img.height() * scale));
    padX = (kInputSize - newW) / 2;
    padY = (kInputSize - newH) / 2;

    const QImage resized = img.scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                               .convertToFormat(QImage::Format_RGB888);

    tensor.assign(3 * kInputSize * kInputSize, 114.0f / 255.0f);

    const int plane = kInputSize * kInputSize;
    for (int y = 0; y < newH; ++y) {
        const uchar *line = resized.constScanLine(y);
        for (int x = 0; x < newW; ++x) {
            const uchar *px = line + x * 3;
            const int dst = (y + padY) * kInputSize + (x + padX);
            tensor[0 * plane + dst] = px[0] / 255.0f;  // R
            tensor[1 * plane + dst] = px[1] / 255.0f;  // G
            tensor[2 * plane + dst] = px[2] / 255.0f;  // B
        }
    }
}

void Detector::postprocess(const float *outData, int numDetections,
                           float scale, int padX, int padY)
{
    detections_.clear();

    const float imgW = static_cast<float>(frame_.width());
    const float imgH = static_cast<float>(frame_.height());

    for (int i = 0; i < numDetections; ++i) {
        const float *row = outData + i * 6;
        const float score = row[4];
        if (score < kConfThreshold)
            continue;

        Detection det;
        det.x1 = std::clamp((row[0] - padX) / scale, 0.0f, imgW);
        det.y1 = std::clamp((row[1] - padY) / scale, 0.0f, imgH);
        det.x2 = std::clamp((row[2] - padX) / scale, 0.0f, imgW);
        det.y2 = std::clamp((row[3] - padY) / scale, 0.0f, imgH);
        det.score = score;
        det.classId = static_cast<int>(row[5]);

        detections_.append(det);
    }
}