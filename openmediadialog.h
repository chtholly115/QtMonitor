#ifndef OPENMEDIADIALOG_H
#define OPENMEDIADIALOG_H

#include "mediasourcemanager.h"
#include <QDialog>

namespace Ui {
class OpenMediaDialog;
}

class OpenMediaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenMediaDialog(QWidget *parent = nullptr);
    ~OpenMediaDialog();

    void showTab(int index);
    void setMediaSourceManager(MediaSourceManager *mediaSourceManager);

private:
    Ui::OpenMediaDialog *ui;
    MediaSourceManager *mediaSourceManager_ = nullptr;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
};

#endif // OPENMEDIADIALOG_H
