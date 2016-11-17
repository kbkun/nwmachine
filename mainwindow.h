
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtNetwork>

#include "qcustomplot.h"
#include "kama.pb.h"

namespace Ui {
class MainWindow;
}

class FrameHandler;
class ControlServer;
class SettingsDialog;
class LogWindow;
class GuanoWorkDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    FrameHandler* frameHandler() { return mFrameHandler; }
    QSerialPort* serial() { return mSerial; }

private:
    Ui::MainWindow *ui;
    FrameHandler *mFrameHandler;
    QSerialPort* mSerial;
    QTcpSocket* tcpSocket;
    quint16 nextBlockSize;
    int timerId;
    SettingsDialog *setDialog;
    LogWindow *logWindow;
    GuanoWorkDialog *guanoWorkDiaolog;
    QStandardItemModel *modelHeader;
    QByteArray dataBuffer;

    QLineEdit* workNum;
    QLineEdit* launchNum;
    QPushButton* mainWorkPB;
    QPushButton* calibrPB;
    QRadioButton* isStartRB;
    QRadioButton* isEndRB;

    QLabel* ledRS;
    QLabel* labelRS;
    QLabel* ledVC;
    QLabel* labelVC;

    QAction* actionOpen;
    QAction* actionSendFile;
    QAction* actionSaveAs;
    QAction* actionExit;
    QAction* actionGuanoWork;
    QAction* actionClean;
    QAction* actionShowLog;
    QAction* actionConfigure;
    QAction* actionCOMConnect;
    QAction* actionCOMDisconnect;
    QAction* actionServerConnect;
    QAction* actionHelp;
    QAction* actionHelpDesk;
    QAction* actionAbout;

    QMenu* fileMenu;
    QMenu* settingsMenu;
    QMenu* serviceMenu;
    QMenu* helpMenu;

    QToolBar* mToolBar;

    QTableView* tableViewMeasure;
    QCustomPlot* widgetDistatnce;
    QCustomPlot* widgetAzimuth;
    QCustomPlot* widgetElevation;
    QLabel* currentDistance;
    QLabel* currentAzimuth;
    QLabel* currentElevation;

    void createActions();
    void createMenu();
    void createToolBar();
    void createPanels();
    void createStatusBar();
    void createHeader();
    void createGraphAxis();
    bool askOnClose();

private slots:
    void addToBuffer(const unsigned char ch);
    void viewFile();
    void sendFile(const QString f);
    void slotSendFile();
    void handleStartSession();
    void handleEndSession();
    void openSerialPort();
    void closeSerialPort();
    void readActivated();
    void handleError(QSerialPort::SerialPortError error);
    void handleError(QString error);
    void handleError(QAbstractSocket::SocketError);
    void connectToServer();
    void slotConnected();
    void slotDisconnected();
    void slotReadyRead();
    void sendMessage(const kama::protocol::Envelope &msg);
    void print(const kama::protocol::Envelope& msg);
    void createGraph(const kama::protocol::Envelope& msg);
    void mainWorkIsChecked(bool);
    void calibrIsChecked(bool);
    void workNumChanged(QString);
    void launchNumChanged(QString);
    void isStartToggled(bool);

    void clearTW();
    void about();
    void helpDesk();
    void help();

signals:
    void rx(unsigned char c, int workNnum = 0, int launchNum = 0, bool mainWork = false);

protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void timerEvent(QTimerEvent*);
};

#endif // MAINWINDOW_H
