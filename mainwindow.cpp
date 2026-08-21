#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QResizeEvent>
#include <QDebug>
#include <QFileDialog>
#include <QUrl>
#include <QVideoFrame>
#include <QImage>
#include <QDir>
#include <QDateTime>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , videoSink_(new QVideoSink(this))
    , openMediaDialog_(new OpenMediaDialog(this))
    , mediaSourceManager_(new MediaSourceManager(this))
    , detector_(new Detector())
    , detectorThread_(new QThread(this))
{
    ui->setupUi(this);
    installEventFilter(this);

    detector_->moveToThread(detectorThread_);
    detectorThread_->start();

    connect(this, &MainWindow::initDetect,
            detector_, &Detector::init);

    emit initDetect();

    connect(this, &MainWindow::startDetect,
            detector_, &Detector::startDetection);

    connect(this, &MainWindow::stopDetect,
            detector_, &Detector::stopDetection);

    mediaSourceManager_->setVideoSink(videoSink_);
    openMediaDialog_->setMediaSourceManager(mediaSourceManager_);

    connect(videoSink_, &QVideoSink::videoFrameChanged,
            ui->openGLWidget, &OpenGLWidget::onvideoFrameChanged);

    connect(videoSink_, &QVideoSink::videoFrameChanged,
            detector_, &Detector::onvideoFrameChanged);

    connect(mediaSourceManager_, &MediaSourceManager::playStateChanged,
            this, [this](bool isPlaying) {
                ui->actionPlay->setText(isPlaying ? tr("暂停") : tr("播放"));
            });

    connect(detector_, &Detector::inferenceStateChanged,
            this, [this](bool isRunning) {
                ui->actionInference->setText(isRunning ? tr("停止推理") : tr("启动推理"));
            });

    connect(detector_, &Detector::detectionResultReady,
            ui->openGLWidget, &OpenGLWidget::ondetectionResultReady);

    connect(mediaSourceManager_, &MediaSourceManager::showError,
            this, [this](const QString &error){
                ui->statusbar->showMessage(error, 3000);
            });

    connect(mediaSourceManager_, &MediaSourceManager::playbackFinished,
            ui->openGLWidget, &OpenGLWidget::clearFrame);
}

MainWindow::~MainWindow()
{
    detectorThread_->quit();
    detectorThread_->wait();
    delete detector_;  // 此时线程已停止，安全删除
    delete ui;
}

void MainWindow::on_actionOpenFile_triggered()
{
    if (!mediaSourceManager_)
        return;

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("打开媒体文件"),
        QString(),
        tr("媒体文件 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.ts *.mpg *.mpeg *.webm *.m4v *.3gp);;所有文件 (*.*)"));

    if (fileName.isEmpty())
        return;

    mediaSourceManager_->openNetworkStream(QUrl::fromLocalFile(fileName));
}

void MainWindow::on_actionOpenNetworkStream_triggered()
{
    openMediaDialog_->showTab(0);
}

void MainWindow::on_actionOpenCaptureDevice_triggered()
{
    openMediaDialog_->showTab(1);
}

void MainWindow::on_actionPlay_triggered()
{
    mediaSourceManager_->play();
}

void MainWindow::on_actionStop_triggered()
{
    mediaSourceManager_->stop();
    ui->openGLWidget->clearFrame();
}

void MainWindow::on_actionRecord_triggered()
{
}

void MainWindow::on_actionScreenshot_triggered()
{
    // 获取合成后的帧（包含检测框和标签）
    QImage frame = ui->openGLWidget->grabCurrentFrame();
    if (frame.isNull()) {
        ui->statusbar->showMessage(tr("没有可截图的帧"), 3000);
        return;
    }
    
    // 获取 exe 所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString screenshotDir = appDir + "/screenshot";
    
    // 创建目录（如果不存在）
    QDir dir(screenshotDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 生成文件名（使用时间戳）
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString fileName = QString("%1/screenshot_%2.png").arg(screenshotDir).arg(timestamp);
    
    // 异步保存图片
    QtConcurrent::run([frame, fileName]() {
        if (!frame.save(fileName)) {
            qWarning() << "截图保存失败:" << fileName;
        }
    });
    
    ui->statusbar->showMessage(tr("截图已保存: %1").arg(fileName), 3000);
}

void MainWindow::on_actionInference_triggered()
{
    if(detector_->isRunning())
        emit stopDetect();
    else
        emit startDetect();
}

void MainWindow::on_actionAbout_triggered()
{

}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this)
    {
        if (event->type() == QEvent::Resize)
        {
            QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
            QSize newSize = resizeEvent->size();
            ui->openGLWidget->resize(newSize);
        }
    }
    return QWidget::eventFilter(watched, event);
}