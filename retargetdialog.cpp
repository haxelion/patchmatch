#include "retargetdialog.h"
#include "ui_retargetdialog.h"

RetargetDialog::RetargetDialog(QWidget *parent, double xscale, double yscale) :
    QDialog(parent),
    ui(new Ui::RetargetDialog)
{
    ui->setupUi(this);
    ui->xscale_box->setValue(xscale);
    ui->yscale_box->setValue(yscale);
}

RetargetDialog::~RetargetDialog()
{
    delete ui;
}

double RetargetDialog::getXscale()
{
    return ui->xscale_box->value();
}

double RetargetDialog::getYscale()
{
    return ui->yscale_box->value();
}
