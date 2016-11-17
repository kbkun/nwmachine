#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QObject>
#include <QDialog>

namespace Ui {
    class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT
public:
    explicit LogWindow(QWidget *parent = 0);
    Ui::LogWindow* ui;
    void log(const QString& str);

signals:

public slots:

};

#endif // LOGWINDOW_H
