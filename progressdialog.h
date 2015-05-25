#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <cmath>

namespace Ui {
class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();

private:
    Ui::ProgressDialog *ui;

public slots:
    void setProgress(double progress);

signals:
    void cancel();

private slots:
    void on_cancelButton_clicked();

};

#endif // PROGRESSDIALOG_H
