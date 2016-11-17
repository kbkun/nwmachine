#include "guanoworkdialog.h"
#include "ui_guanoworkdialog.h"

#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

GuanoWorkDialog::GuanoWorkDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GuanoWorkDialog)
    , modelHeader(new QStandardItemModel())
{
    ui->setupUi(this);
    setWindowTitle(tr("Отчёт о работе"));

    connect(ui->sendPushButton, SIGNAL(clicked()), SLOT(createReport()));
    connect(ui->cancelPushButton, SIGNAL(clicked()), this, SLOT(close()));
}

GuanoWorkDialog::~GuanoWorkDialog()
{
    delete ui;
}

void GuanoWorkDialog::createHeader()
{
    QStringList horizontalHeader;
    horizontalHeader.append(tr("Время"));
    horizontalHeader.append((tr("Номер пуска")));
    horizontalHeader.append(tr("Сбой"));

    modelHeader->setHorizontalHeaderLabels(horizontalHeader);

    ui->guanoTableView->setModel(modelHeader);
}

void GuanoWorkDialog::refresh()
{
    modelHeader->clear();
    createHeader();
    QSettings settings("nwmachine", "settings");
    QString saveDir = settings.value("/main/save_dir").toString();
    QDir dir(saveDir);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time);
    int row = modelHeader->rowCount();

    QFileInfoList dirContent = dir.entryInfoList();

    foreach (QFileInfo info, dirContent) {
        modelHeader->setItem(row, 0, new QStandardItem(info.created().toString("yyyy.MM.dd hh:mm:ss")));
        modelHeader->setItem(row, 1, new QStandardItem(info.fileName()));
        QStandardItem *checkItem = new QStandardItem(true);
        checkItem->setCheckable(true);
        checkItem->setEnabled(true);
        modelHeader->setItem(row, 2, checkItem);
        ++row;
    }
    ui->guanoTableView->resizeColumnsToContents();
}

void GuanoWorkDialog::createReport()
{
    QStringList guanoList;
    int row  = modelHeader->rowCount();
    for(int i = 0; i < row; ++i) {
        QModelIndex guano = ui->guanoTableView->model()->index(i, 2);
        if(guano.data(Qt::CheckStateRole) == Qt::Checked) {
            QModelIndex guanoName = ui->guanoTableView->model()->index(i, 1);
            guanoList.append(guanoName.data().toString().append("s"));
        }
    }
    QDir dir = QDir(QDir::homePath());
    QString fileName = dir.absoluteFilePath("results.txt");
    QFile f(fileName);
    if(!f.open(QIODevice::WriteOnly)) {
        QString errStr = QTime::currentTime().toString() + tr(" Ошибка: Не могу открыть файл ") + f.fileName() + " . " + f.errorString();
        emit error(errStr);
        close();
    }
    QTextStream s(&f);
    foreach (QString str, guanoList) {
        s << str << "\n";
    }
    f.close();
    emit report(fileName);
    close();
}
