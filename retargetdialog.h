#ifndef RETARGETDIALOG_H
#define RETARGETDIALOG_H

#include <QDialog>

namespace Ui {
class RetargetDialog;
}

class RetargetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RetargetDialog(QWidget *parent = 0, double xscale = 1.0, double yscale = 1.0);
    ~RetargetDialog();
    double getXscale();
    double getYscale();

private:
    Ui::RetargetDialog *ui;
};

#endif // RETARGETDIALOG_H
