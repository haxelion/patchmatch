#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent, int patch_w, int patchmatch_iterations, int em_iterations) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->patch_w_box->setValue(patch_w);
    ui->patchmatch_iterations_box->setValue(patchmatch_iterations);
    ui->em_iterations_box->setValue(em_iterations);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

int SettingsDialog::getPatchW()
{
    return ui->patch_w_box->value();
}

int SettingsDialog::getPatchmatchIterations()
{
    return ui->patchmatch_iterations_box->value();
}

int SettingsDialog::getEMIterations()
{
    return ui->em_iterations_box->value();
}
