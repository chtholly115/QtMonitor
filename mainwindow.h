#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "openmediadialog.h"
#include "mediasourcemanager.h"
#include "detector.h"
#include <QMainWindow>
#include <QVideoSink>
#include <QThread>
#include <QEvent>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void on_actionOpenFile_triggered();
    void on_actionOpenNetworkStream_triggered();
    void on_actionOpenCaptureDevice_triggered();

    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionRecord_triggered();

    void on_actionScreenshot_triggered();
    void on_actionInference_triggered();

    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;

    OpenMediaDialog *openMediaDialog_ = nullptr;
    MediaSourceManager *mediaSourceManager_ = nullptr;
    QVideoSink *videoSink_ = nullptr;
    Detector *detector_ = nullptr;
    QThread *detectorThread_ = nullptr;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void initDetect();
    void startDetect();
    void stopDetect();
};
#endif // MAINWINDOW_H
