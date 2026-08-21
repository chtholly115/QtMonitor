#include "openmediadialog.h"
#include "ui_openmediadialog.h"

#include <QMediaDevices>
#include <QUrl>

OpenMediaDialog::OpenMediaDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenMediaDialog)
{
    ui->setupUi(this);

    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &camera : cameras)
        ui->videoComboBox->addItem(camera.description(), QVariant::fromValue(camera));
}

OpenMediaDialog::~OpenMediaDialog()
{
    delete ui;
}

void OpenMediaDialog::showTab(int index)
{
    ui->tabWidget->setCurrentIndex(index);
    show();
}

void OpenMediaDialog::setMediaSourceManager(MediaSourceManager *mediaSourceManager)
{
    mediaSourceManager_ = mediaSourceManager;
}

void OpenMediaDialog::on_buttonBox_accepted()
{
    if (mediaSourceManager_)
    {
        int index = ui->tabWidget->currentIndex();
        switch (index) {
        case 0:
            mediaSourceManager_->openNetworkStream(QUrl(ui->urlLineEdit->text()));
            break;
        case 1:
            mediaSourceManager_->openCaptureDevice(
                ui->videoComboBox->currentData().value<QCameraDevice>());
            break;
        default:
            break;
        }
    }
    hide();
}

void OpenMediaDialog::on_buttonBox_rejected()
{
    hide();
}