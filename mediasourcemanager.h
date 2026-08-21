#ifndef MEDIASOURCEMANAGER_H
#define MEDIASOURCEMANAGER_H

#include <QObject>
#include <QUrl>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QMediaPlayer>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QPointer>

class MediaSourceManager : public QObject
{
    Q_OBJECT
public:
    explicit MediaSourceManager(QObject *parent = nullptr);
    ~MediaSourceManager();

    void play();
    void stop();

    void setVideoSink(QVideoSink *videoSink);

public slots:
    void openNetworkStream(const QUrl &url);
    void openCaptureDevice(const QCameraDevice &cameraDevice);

private:
    enum class ActiveSource { None, Network, Capture };

    void stopSources();

    QMediaPlayer *mediaPlayer_ = nullptr;
    QCamera *camera_ = nullptr;
    QMediaCaptureSession *captureSession_ = nullptr;
    QPointer<QVideoSink> videoSink_;
    ActiveSource activeSource_   = ActiveSource::None;

signals:
    void playStateChanged(bool isPlaying);
    void showError(const QString &error);
    void playbackFinished();
};

#endif // MEDIASOURCEMANAGER_H
