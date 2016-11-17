#ifndef GUANOWORKDIALOG_H
#define GUANOWORKDIALOG_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class GuanoWorkDialog;
}

class GuanoWorkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GuanoWorkDialog(QWidget *parent = 0);
    ~GuanoWorkDialog();

private:
    Ui::GuanoWorkDialog *ui;
    QStandardItemModel *modelHeader;
    void createHeader();

private slots:
    void createReport();

public slots:
    void refresh();

signals:
    void error(QString);
    void report(QString);
};

#endif // GUANOWORKDIALOG_H
