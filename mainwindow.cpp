#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    source = NULL;
    target = NULL;
    zones = new std::vector<Zone*>();
    ui->setupUi(this);
    image_view = ui->centralWidget;
    image_view->setZones(zones);
    active_tool = TOOL_NONE;
    progress_dialog = NULL;
    setting_dialog = NULL;
    patch_w = 7;
    patchmatch_iterations = 5;
    em_iterations = 8;
    xscale = 1.0;
    yscale = 1.0;
}

MainWindow::~MainWindow()
{
    delete ui;
    if(source != NULL)
        delete source;
    if(target != NULL)
        delete target;
    for(unsigned int i = 0; i < zones->size(); i++)
        delete zones->at(i);
    delete zones;
}

void MainWindow::on_actionOpen_triggered()
{
    QString file_name = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if(file_name.isNull() == false)
    {
        image_view->setImages(NULL, NULL);
        if(source != NULL)
            delete source;
        if(target != NULL)
            delete target;
        for(unsigned int i = 0; i < zones->size(); i++)
            delete zones->at(i);
        zones->clear();
        source = new QImage(QImage(file_name).convertToFormat(QImage::Format_RGB32));
        if(source->isNull())
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not open image"));
        }
        else
        {
            target = new QImage(*source);
            image_view->setImages(source, target);
            this->file_name = file_name;
            xscale = 1.0;
            yscale = 1.0;
        }
    }

}

void MainWindow::on_actionSave_As_triggered()
{
    QString file_name = QFileDialog::getSaveFileName(this, tr("Save Image"),
        "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if(file_name.isNull() == false)
        saveImage(file_name);
}

void MainWindow::saveImage(QString file_name)
{
    if(source->save(file_name) == false)
        QMessageBox::critical(this, tr("Error"), tr("Could not save image"));
}

void MainWindow::setActiveTool(Tool tool)
{
    if(tool == active_tool)
        return;

    if(active_tool == TOOL_MOVE)
        ui->actionMove->setDisabled(false);
    else if(active_tool == TOOL_DELETE)
        ui->actionDelete->setDisabled(false);
    else if(active_tool == TOOL_RESHUFFLE_RECTANGLE)
        ui->actionReshuffleRectangle->setDisabled(false);
    else if(active_tool == TOOL_RESHUFFLE_FREE_HAND)
        ui->actionReshuffleFreeHand->setDisabled(false);
    else if(active_tool == TOOL_REPLACE_RECTANGLE)
        ui->actionReplaceRectangle->setDisabled(false);
    else if(active_tool == TOOL_REPLACE_FREE_HAND)
        ui->actionReplaceFreeHand->setDisabled(false);

    if(tool == TOOL_MOVE)
        ui->actionMove->setDisabled(true);
    else if(tool == TOOL_DELETE)
        ui->actionDelete->setDisabled(true);
    else if(tool == TOOL_RESHUFFLE_RECTANGLE)
        ui->actionReshuffleRectangle->setDisabled(true);
    else if(tool == TOOL_RESHUFFLE_FREE_HAND)
        ui->actionReshuffleFreeHand->setDisabled(true);
    else if(tool == TOOL_REPLACE_RECTANGLE)
        ui->actionReplaceRectangle->setDisabled(true);
    else if(tool == TOOL_REPLACE_FREE_HAND)
        ui->actionReplaceFreeHand->setDisabled(true);

    active_tool = tool;
    image_view->setActiveTool(tool);
}

void MainWindow::on_actionSave_triggered()
{
    saveImage(this->file_name);
}

void MainWindow::on_actionMove_triggered()
{
    setActiveTool(TOOL_MOVE);
}

void MainWindow::on_actionDelete_triggered()
{
    setActiveTool(TOOL_DELETE);
}

void MainWindow::on_actionReshuffleRectangle_triggered()
{
    setActiveTool(TOOL_RESHUFFLE_RECTANGLE);
}

void MainWindow::on_actionReshuffleFreeHand_triggered()
{
    setActiveTool(TOOL_RESHUFFLE_FREE_HAND);
}

void MainWindow::on_actionReplaceRectangle_triggered()
{
    setActiveTool(TOOL_REPLACE_RECTANGLE);
}

void MainWindow::on_actionReplaceFreeHand_triggered()
{
    setActiveTool(TOOL_REPLACE_FREE_HAND);
}

void MainWindow::on_actionProcess_triggered()
{
    worker = new PatchMatchAlgo(source, target, zones, xscale, yscale, patch_w, patchmatch_iterations, em_iterations);
    progress_dialog = new ProgressDialog(this);
    progress_dialog->show();
    connect(worker, static_cast<void (PatchMatchAlgo::*)(double)>(&PatchMatchAlgo::progressed), progress_dialog, &ProgressDialog::setProgress);
    connect(worker, static_cast<void (PatchMatchAlgo::*)(QImage)>(&PatchMatchAlgo::progressed), this, &MainWindow::onWorkerProgress);
    connect(worker, &PatchMatchAlgo::finished, this, &MainWindow::onWorkerFinished);
    connect(progress_dialog, &ProgressDialog::cancel, worker, &PatchMatchAlgo::stop);
    connect(progress_dialog, &ProgressDialog::rejected, worker, &PatchMatchAlgo::stop);
    worker->start();
    image_view->setWorking(true);
}

void MainWindow::onWorkerProgress(QImage progress)
{
    image_view->setImages(NULL, NULL);
    delete target;
    target = new QImage(progress);
    image_view->setImages(source, target);
}

void MainWindow::onWorkerFinished()
{
    progress_dialog->accept();
    progress_dialog->close();
    progress_dialog->deleteLater();
    image_view->setWorking(false);
    if(worker->canceled)
    {
        image_view->setImages(NULL, NULL);
        delete target;
        target = new QImage(*source);
        image_view->setImages(source, target);
    }
    else
    {
        for(unsigned int i = 0; i < zones->size(); i++)
            delete zones->at(i);
        zones->clear();
        image_view->setImages(NULL, NULL);
        delete source;
        source = new QImage(*target);
        image_view->setImages(source, target);
        xscale = 1.0;
        yscale = 1.0;
        image_view->setRetargetScales(xscale, yscale);
    }
    delete worker;
}

void MainWindow::on_actionSettings_triggered()
{
    setting_dialog = new SettingsDialog(this, patch_w, patchmatch_iterations, em_iterations);
    if(setting_dialog->exec() == QDialog::Accepted)
    {
        patch_w = setting_dialog->getPatchW();
        patchmatch_iterations = setting_dialog->getPatchmatchIterations();
        em_iterations = setting_dialog->getEMIterations();
    }
    setting_dialog->deleteLater();
}

void MainWindow::on_actionRetarget_triggered()
{
    retarget_dialog = new RetargetDialog(this, xscale, yscale);
    if(retarget_dialog->exec() == QDialog::Accepted)
    {
        xscale = retarget_dialog->getXscale();
        yscale = retarget_dialog->getYscale();
        image_view->setRetargetScales(xscale, yscale);
    }
    retarget_dialog->deleteLater();
}
