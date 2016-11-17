#include "logwindow.h"
#include "ui_logwindow.h"

LogWindow::LogWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Журнал работы"));

    connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));
}

void LogWindow::log(const QString &str)
{
    ui->logTextEdit->append(str);
}
