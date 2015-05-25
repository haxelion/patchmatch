#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>

#include "imageview.h"
#include "zone.h"
#include "patchmatchalgo.h"
#include "progressdialog.h"
#include "settingsdialog.h"
#include "retargetdialog.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_actionSave_As_triggered();

    void on_actionSave_triggered();

    void on_actionMove_triggered();

    void on_actionDelete_triggered();

    void on_actionReshuffleRectangle_triggered();

    void on_actionReshuffleFreeHand_triggered();

    void on_actionReplaceRectangle_triggered();

    void on_actionReplaceFreeHand_triggered();

    void onWorkerFinished();

    void onWorkerProgress(QImage progress);

    void on_actionProcess_triggered();

    void on_actionSettings_triggered();

    void on_actionRetarget_triggered();

private:
    Ui::MainWindow *ui;
    QImage *source, *target;
    std::vector<Zone*> *zones;
    QString file_name;
    ImageView *image_view;
    Tool active_tool;
    Move current_move;
    PatchMatchAlgo *worker;
    ProgressDialog *progress_dialog;
    SettingsDialog *setting_dialog;
    RetargetDialog *retarget_dialog;
    int patch_w;
    int patchmatch_iterations;
    int em_iterations;
    double xscale, yscale;

    void saveImage(QString file_name);
    void setActiveTool(Tool tool);
};

#endif // MAINWINDOW_H
