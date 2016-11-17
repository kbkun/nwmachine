#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtNetwork>
#include <QtSql/QPSQLDriver>

namespace Ui {
    class SettingsDialog;
}


class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    Ui::SettingsDialog *ui;

private:
    QSettings mSettings;

private slots:
    void setSaveDir();
    void apply();

private:
   void saveSettings();
   void readSettings();
   void comPortsInfo();
   void comPortParametres();
};

#endif // SETTINGSDIALOG_H
