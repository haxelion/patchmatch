#include "progressdialog.h"
#include "ui_progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}


void ProgressDialog::setProgress(double progress)
{
    ui->progressBar->setValue(round(progress*100));
}

void ProgressDialog::on_cancelButton_clicked()
{
    ui->cancelButton->setText(tr("Canceling ..."));
    ui->cancelButton->setDisabled(true);
    emit cancel();
}
