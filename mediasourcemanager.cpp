#include "mediasourcemanager.h"

#include <QDebug>

MediaSourceManager::MediaSourceManager(QObject *parent)
    : QObject{parent}
    , mediaPlayer_(new QMediaPlayer(this))
    , captureSession_(new QMediaCaptureSession(this))
{
    connect(mediaPlayer_, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
                emit playStateChanged(state == QMediaPlayer::PlayingState);
            });
    connect(mediaPlayer_, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia) {
                    emit playbackFinished();
                }
            });
}

MediaSourceManager::~MediaSourceManager()
{
    stopSources();
}

void MediaSourceManager::play()
{
    switch (activeSource_) {
    case ActiveSource::Network:
        if (mediaPlayer_->playbackState() == QMediaPlayer::PlayingState)
            mediaPlayer_->pause();
        else
            mediaPlayer_->play();
        break;
    case ActiveSource::Capture:
        if (camera_) {
            if (camera_->isActive())
                camera_->stop();
            else
                camera_->start();
        }
        break;
    case ActiveSource::None:
        emit showError("[ERROR] No active source to play/pause");
        break;
    }
}

void MediaSourceManager::stop()
{
    switch (activeSource_) {
    case ActiveSource::Network:
        mediaPlayer_->stop();
        break;
    case ActiveSource::Capture:
        if (camera_)
            camera_->stop();
        break;
    case ActiveSource::None:
        emit showError("[ERROR] No active source to stop");
        break;
    }
}

void MediaSourceManager::setVideoSink(QVideoSink *videoSink)
{
    videoSink_ = videoSink;

    switch (activeSource_) {
    case ActiveSource::Network:
        mediaPlayer_->setVideoOutput(videoSink);
        break;
    case ActiveSource::Capture:
        captureSession_->setVideoSink(videoSink);
        break;
    case ActiveSource::None:
        break;
    }
}

void MediaSourceManager::openNetworkStream(const QUrl &url)
{
    if (videoSink_.isNull()) {
        qWarning() << "[MediaSourceManager] videoSink is not set, cannot play network stream";
        return;
    }
    if (url.isEmpty()) {
        emit showError(QString("[ERROR] Invalid URL"));
        qWarning() << "[MediaSourceManager] empty url, cannot play network stream";
        return;
    }

    stopSources();

    mediaPlayer_->setVideoOutput(videoSink_);
    mediaPlayer_->setSource(url);
    mediaPlayer_->play();

    activeSource_ = ActiveSource::Network;
}

void MediaSourceManager::openCaptureDevice(const QCameraDevice &device)
{
    if (videoSink_.isNull()) {
        qWarning() << "[MediaSourceManager] videoSink is not set, cannot open capture device";
        return;
    }
    if (device.isNull()) {
        emit showError(QString("[ERROR] Invalid capture device"));
        qWarning() << "[MediaSourceManager] invalid capture device";
        return;
    }

    stopSources();

    delete camera_;
    camera_ = new QCamera(device, this);

    connect(camera_, &QCamera::activeChanged, this,
            [this](bool active) {
                emit playStateChanged(active);
            });

    captureSession_->setCamera(camera_);
    captureSession_->setVideoSink(videoSink_);
    camera_->start();

    activeSource_ = ActiveSource::Capture;
}

void MediaSourceManager::stopSources()
{
    mediaPlayer_->stop();
    mediaPlayer_->setSource(QUrl());
    mediaPlayer_->setVideoOutput(nullptr);

    if (camera_) {
        camera_->stop();
        captureSession_->setCamera(nullptr);
        captureSession_->setVideoSink(nullptr);
    }

    activeSource_ = ActiveSource::None;
    emit playStateChanged(false);
}
