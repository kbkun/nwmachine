#include "settingsdialog.h"
#include "ui_mainwindow.h"
#include "ui_settingsdialog.h"

#include <QFileDialog>


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
  , mSettings("nwmachine", "settings")
{
    ui->setupUi(this);
    setWindowTitle(tr("Настройки"));
    ui->toolBox->setCurrentIndex(0);

    connect(ui->saveDirToolButton, SIGNAL(clicked()), SLOT(setSaveDir()));
    connect(ui->cancelButton, SIGNAL(clicked()),this, SLOT(close()));
    connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(apply()));

    comPortsInfo();
    comPortParametres();
    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setSaveDir()
{
    QString saveDir = QFileDialog::getExistingDirectory(this, tr("Выбрать каталог"), QDir::home().absolutePath());
    ui->saveDirLineEdit->setText(saveDir);
}

void SettingsDialog::apply()
{
    saveSettings();
    hide();
    ui->toolBox->setCurrentIndex(0);
}

void SettingsDialog::saveSettings()
{
mSettings.beginGroup("main");
    mSettings.setValue("mp_num", ui->mpNumLineEdit->text());
    mSettings.setValue("save_dir", ui->saveDirLineEdit->text());
mSettings.endGroup();

mSettings.beginGroup("network");
    mSettings.setValue("dest_ip", ui->destIPLineEdit->text());
    mSettings.setValue("dest_port",ui->destPortLineEdit->text());
mSettings.endGroup();

mSettings.beginGroup("COM");
    mSettings.setValue("device_idx",ui->serialPortInfoListBox->currentIndex());
    mSettings.setValue("baud_rate_idx",ui->baudRateBox->currentIndex());
    mSettings.setValue("data_bits_idx",ui->dataBitsBox->currentIndex());
    mSettings.setValue("stop_bit_idx",ui->stopBitsBox->currentIndex());
    mSettings.setValue("parity_idx",ui->parityBox->currentIndex());
    mSettings.setValue("flow_idx",ui->flowControlBox->currentIndex());

    mSettings.setValue("device",ui->serialPortInfoListBox->currentText());

    mSettings.setValue("baud_rate",
             static_cast<QSerialPort::BaudRate>(ui->baudRateBox->itemData(ui->baudRateBox->currentIndex()).toInt()));

    mSettings.setValue("data_bits",
             static_cast<QSerialPort::DataBits>(ui->dataBitsBox->itemData(ui->dataBitsBox->currentIndex()).toInt()));

    mSettings.setValue("stop_bit",
             static_cast<QSerialPort::StopBits>(ui->stopBitsBox->itemData(ui->stopBitsBox->currentIndex()).toInt()));

    mSettings.setValue("parity",
             static_cast<QSerialPort::Parity>(ui->parityBox->itemData(ui->parityBox->currentIndex()).toInt()));

    mSettings.setValue("flow",
             static_cast<QSerialPort::FlowControl>(ui->flowControlBox->itemData(ui->flowControlBox->currentIndex()).toInt()));
mSettings.endGroup();
}

void SettingsDialog::readSettings()
{
mSettings.beginGroup("main");
    ui->mpNumLineEdit->setText(mSettings.value("mp_num").toString());
    ui->saveDirLineEdit->setText(mSettings.value("save_dir").toString());
mSettings.endGroup();

mSettings.beginGroup("network");
    ui->destIPLineEdit->setText(mSettings.value("dest_ip").toString());
    ui->destPortLineEdit->setText(mSettings.value("dest_port").toString());
mSettings.endGroup();

mSettings.beginGroup("COM");
    ui->serialPortInfoListBox->setCurrentIndex(mSettings.value("device_idx").toInt());
    ui->baudRateBox->setCurrentIndex(mSettings.value("baud_rate_idx").toInt());
    ui->dataBitsBox->setCurrentIndex(mSettings.value("data_bits_idx").toInt());
    ui->parityBox->setCurrentIndex(mSettings.value("parity_idx").toInt());
    ui->stopBitsBox->setCurrentIndex(mSettings.value("stop_bit_idx").toInt());
    ui->flowControlBox->setCurrentIndex(mSettings.value("flow_idx").toInt());
mSettings.endGroup();
}

void SettingsDialog::comPortsInfo()
{
    ui->serialPortInfoListBox->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QStringList list;
        list << info.portName()
          << info.systemLocation()
          << (info.isBusy() ? QObject::tr("Занят") : QObject::tr("Свободен"));
        ui->serialPortInfoListBox->addItem(list.first(), list);
    }
}

void SettingsDialog::comPortParametres()
{
    ui->baudRateBox->addItem(QStringLiteral("1200"), QSerialPort::Baud1200);
    ui->baudRateBox->addItem(QStringLiteral("2400"), QSerialPort::Baud2400);
    ui->baudRateBox->addItem(QStringLiteral("4800"), QSerialPort::Baud4800);
    ui->baudRateBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->baudRateBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->baudRateBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->baudRateBox->addItem(QStringLiteral("57600"), QSerialPort::Baud57600);
    ui->baudRateBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);

    ui->dataBitsBox->addItem(QStringLiteral("5"), QSerialPort::Data5);
    ui->dataBitsBox->addItem(QStringLiteral("6"), QSerialPort::Data6);
    ui->dataBitsBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
    ui->dataBitsBox->addItem(QStringLiteral("8"), QSerialPort::Data8);
    ui->dataBitsBox->setCurrentIndex(3);

    ui->parityBox->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    ui->parityBox->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    ui->parityBox->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);

    ui->stopBitsBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
    #ifdef Q_OS_WIN
        ui->stopBitsBox->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
    #endif
    ui->stopBitsBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    ui->flowControlBox->addItem(QStringLiteral("None"), QSerialPort::NoFlowControl);
    ui->flowControlBox->addItem(QStringLiteral("RTS/CTS"), QSerialPort::HardwareControl);
    ui->flowControlBox->addItem(QStringLiteral("XON/XOFF"), QSerialPort::SoftwareControl);
}


