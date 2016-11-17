#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "guanoworkdialog.h"
#include "ui_guanoworkdialog.h"
#include "logwindow.h"
#include "ui_logwindow.h"
#include "framehandler.h"
#include "helpers.h"
#include "convertation.h"

#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , mFrameHandler(new FrameHandler(this))
  , mSerial(new QSerialPort(this))
  , tcpSocket(new QTcpSocket(this))
  , timerId(-1)
  , setDialog(new SettingsDialog(this))
  , logWindow(new LogWindow(this))
  , guanoWorkDiaolog(new GuanoWorkDialog(this))
  , modelHeader(new QStandardItemModel())

{
    ui->setupUi(this);
    resize(880, 710);

    setWindowTitle(tr("ПО блока КНВ-010"));

    createActions();
    createMenu();
    createToolBar();
    createPanels();
    createHeader();
    createGraphAxis();
    createStatusBar();

//    mStreamRecorder = new StreamRecorder(this);

    connect(mainWorkPB, SIGNAL(toggled(bool)), SLOT(mainWorkIsChecked(bool)));
    connect(calibrPB, SIGNAL(toggled(bool)), SLOT(calibrIsChecked(bool)));
    connect(workNum, SIGNAL(textChanged(QString)), SLOT(workNumChanged(QString)));
    connect(launchNum, SIGNAL(textChanged(QString)), SLOT(launchNumChanged(QString)));
    connect(isStartRB, SIGNAL(toggled(bool)), SLOT(isStartToggled(bool)));

    connect(tcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(tcpSocket, SIGNAL(disconnected()), SLOT(slotDisconnected()));
    connect(tcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),SLOT(handleError(QAbstractSocket::SocketError)));

    connect(this, SIGNAL(rx(unsigned char, int, int, bool)), mFrameHandler, SLOT(receiveChar(unsigned char, int, int, bool)));
    connect(this, SIGNAL(rx(unsigned char)), SLOT(addToBuffer(unsigned char)));

    connect(mFrameHandler, SIGNAL(readyToSend(const kama::protocol::Envelope&)), SLOT(print(const kama::protocol::Envelope&)));
    connect(mFrameHandler, SIGNAL(readyToSend(const kama::protocol::Envelope&)), SLOT(sendMessage(const kama::protocol::Envelope&)));
    connect(mFrameHandler, SIGNAL(readyToSend(const kama::protocol::Envelope&)), SLOT(createGraph(const kama::protocol::Envelope&)));
    connect(mFrameHandler, SIGNAL(startSession()), SLOT(handleStartSession()));
    connect(mFrameHandler, SIGNAL(endSession()), SLOT(handleEndSession()));

    connect(mSerial, SIGNAL(readyRead()), SLOT(readActivated()));

    connect(guanoWorkDiaolog, SIGNAL(report(QString)), SLOT(sendFile(QString)));
    connect(guanoWorkDiaolog, SIGNAL(error(QString)), SLOT(handleError(QString)));

    connectToServer();
    openSerialPort();
}

MainWindow::~MainWindow()
{
    serial()->close();
    delete ui;
}


void MainWindow::createActions()
{
    actionOpen = new QAction(QIcon(":/images/open.png"), tr("&Открыть"), this);
    actionOpen->setShortcut(tr("Ctrl+O"));
    actionOpen->setStatusTip(tr("Открыть файл"));
    connect(actionOpen, SIGNAL(triggered()), SLOT(viewFile()));

    actionSendFile = new QAction(QIcon(":/images/svn-commit.png"), tr("&Передать файл"), this);
    actionSendFile->setShortcut(tr("Ctrl+S"));
    actionSendFile->setStatusTip(tr("Передать файл на ВЦ"));
    connect(actionSendFile, SIGNAL(triggered()), SLOT(slotSendFile()));

//    actionSaveAs = new QAction(QIcon(":/images/save-as.png"), tr("&Сохранить как..."), this);
//    actionSaveAs->setShortcut(tr("Ctrl+S"));
//    actionSaveAs->setStatusTip(tr("Сохранить файл как..."));

    actionExit = new QAction(QIcon(":/images/application-exit.png"), tr("В&ыход"), this);
    actionExit->setShortcut(tr("Ctrl+Q"));
    actionExit->setStatusTip("Закрыть программу");
    connect(actionExit, SIGNAL(triggered()), SLOT(close()));

    actionConfigure = new QAction(QIcon(":/images/settings.png"), tr("&Параметры"), this);
    actionConfigure->setStatusTip(tr("Изменить параметры конфигурации"));
    connect(actionConfigure, SIGNAL(triggered()),setDialog, SLOT(show()));

    actionCOMConnect = new QAction(QIcon(":/images/connect.png"), tr("&Открыть COM-порт"), this);
    actionCOMConnect->setStatusTip(tr("Установить заданные в настройках параметры COM-порта и установить соединение с КНВ008"));
    connect(actionCOMConnect, SIGNAL(triggered()),SLOT(openSerialPort()));

    actionCOMDisconnect = new QAction(QIcon(":/images/disconnect.png"), tr("&Закрыть COM-порт"), this);
    actionCOMDisconnect->setStatusTip(tr("Разорвать соединение с КНВ008"));
    connect(actionCOMDisconnect, SIGNAL(triggered()), SLOT(closeSerialPort()));

    actionClean = new QAction(QIcon(":/images/clear.png"), tr("О&чистить"), this);
    actionClean->setStatusTip(tr("Очистить таблицу с измерениями"));
    connect(actionClean, SIGNAL(triggered()), SLOT(clearTW()));

    actionShowLog = new QAction(QIcon(":/images/document-edit.png"), tr("Показать &журнал"), this);
    actionShowLog->setShortcut(tr("Ctrl+L"));
    actionShowLog->setStatusTip(tr("Показать журнал работы"));
    connect(actionShowLog, SIGNAL(triggered()), logWindow ,SLOT(show()));

    actionGuanoWork = new QAction(QIcon(":/images/view-calendar-tasks.png"), tr("Отправить отчёт о &работе"), this);
    actionGuanoWork->setShortcut(tr("Ctrl+R"));
    actionGuanoWork->setStatusTip(tr("Отправить отчёт о работе"));
    connect(actionGuanoWork, SIGNAL(triggered()), guanoWorkDiaolog, SLOT(refresh()));
    connect(actionGuanoWork, SIGNAL(triggered()), guanoWorkDiaolog, SLOT(show()));

    actionServerConnect = new QAction(QIcon(":/images/network-wired.png"), tr("Подключиться к &серверу"), this);
    actionServerConnect->setStatusTip(tr("Подключиться к серверу на ВЦ"));
    connect(actionServerConnect,SIGNAL(triggered()), SLOT(connectToServer()));

    actionAbout = new QAction(QIcon(":/images/about.png"), tr("&О программе"), this);
    actionAbout->setStatusTip(tr("Информация о разработчике и версии программы"));
    connect(actionAbout, SIGNAL(triggered()), SLOT(about()));

    actionHelpDesk = new QAction(tr("&Техническая поддержка"), this);
    actionHelpDesk->setStatusTip(tr("Как получить техническую поддержку?"));
    connect(actionHelpDesk, SIGNAL(triggered()), this, SLOT(helpDesk()));

    actionHelp = new QAction(QIcon(":/images/help-contents.png"), tr("&Помощь"), this);
    actionHelp->setShortcut(tr("F1"));
    actionHelp->setStatusTip(tr("Руководство оператора"));
    connect(actionHelp, SIGNAL(triggered()), SLOT(help()));
}

void MainWindow::createMenu()
{
    fileMenu = menuBar()->addMenu(tr("&Файл"));
    fileMenu->addAction(actionOpen);
//    fileMenu->addAction(actionSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(actionSendFile);
    fileMenu->addSeparator();
    fileMenu->addAction(actionExit);

    settingsMenu = menuBar()->addMenu(tr("&Настройки"));
    settingsMenu->addAction(actionConfigure);

    serviceMenu = menuBar()->addMenu(tr("&Сервис"));
    serviceMenu->addAction(actionGuanoWork);
    serviceMenu->addAction(actionShowLog);
    serviceMenu->addAction(actionClean);
    serviceMenu->addSeparator();
    serviceMenu->addAction(actionCOMConnect);
    serviceMenu->addAction(actionCOMDisconnect);
    serviceMenu->addAction(actionServerConnect);

    helpMenu = menuBar()->addMenu(tr("&Помощь"));
    helpMenu->addAction(actionHelp);
    helpMenu->addAction(actionHelpDesk);
    helpMenu->addAction(actionAbout);
}

void MainWindow::createToolBar()
{
    mToolBar = new QToolBar();
    mToolBar->addAction(actionOpen);
//    mToolBar->addAction(actionSaveAs);
    mToolBar->addAction(actionSendFile);
    mToolBar->addSeparator();
    mToolBar->addAction(actionConfigure);
    mToolBar->addSeparator();
    mToolBar->addAction(actionGuanoWork);
    mToolBar->addAction(actionShowLog);
    mToolBar->addAction(actionClean);
    mToolBar->addSeparator();
    mToolBar->addAction(actionCOMConnect);
    mToolBar->addAction(actionCOMDisconnect);
    mToolBar->addAction(actionServerConnect);
    mToolBar->addSeparator();
    mToolBar->addAction(actionHelp);
    addToolBar(Qt::TopToolBarArea, mToolBar);
}

void MainWindow::createPanels()
{
    QFont font("url(:/fonts/Arial.ttf)");
    QGridLayout* mainLayout = new QGridLayout();

    QGroupBox* confGroupBox = new QGroupBox(tr("Номер работы/пуска"));
    confGroupBox->setStyleSheet("color: blue");
    QGridLayout* confLayout = new QGridLayout;
    QLabel* workNumLab = new QLabel(tr("Номер работы"));
    workNumLab->setStyleSheet("color: black");
    QLabel* launchNumLab = new QLabel(tr("Номер пуска"));
    launchNumLab->setStyleSheet("color: black");
    workNum = new QLineEdit();
    workNum->setStyleSheet("color: black");
    launchNum = new QLineEdit();
    launchNum->setStyleSheet("color: black");
    QSettings settings("nwmachine", "settings");
    if(settings.contains("/main/work_num")) {
       workNum->setText(settings.value("/main/work_num").toString());
    } else {
        settings.setValue("/main/work_num", 1);
        workNum->setText("1");
    }
    if(settings.contains("/main/launch_num")) {
        launchNum->setText(settings.value("/main/launch_num").toString());
    } else {
        settings.setValue("/main/launch_num", 1);
        launchNum->setText("1");
    }
    settings.setValue("/main/main_work", false);
    settings.setValue("/main/isStart", true);
    confLayout->addWidget(workNumLab, 0, 0);
    confLayout->addWidget(workNum, 0, 1);
    confLayout->addWidget(launchNumLab, 1, 0);
    confLayout->addWidget(launchNum, 1, 1);
    confGroupBox->setLayout(confLayout);

    QGroupBox* mainWorkGroupBox =  new QGroupBox(tr("Режим работы"));
    mainWorkGroupBox->setStyleSheet("color: blue");
    QGridLayout* mainWorkLayout = new QGridLayout();
    mainWorkPB = new QPushButton(tr("Основная работа"));
    mainWorkPB->setStyleSheet("color: black; background-color: lightgray;");
    mainWorkPB->setCheckable(true);
    mainWorkPB->setChecked(false);
    calibrPB = new QPushButton(tr("Калибровка"));
    calibrPB->setStyleSheet("color: black; background-color: lightgreen;");
    calibrPB->setCheckable(true);
    calibrPB->setChecked(true);
    isStartRB = new QRadioButton(tr("Начало"));
    isStartRB->setStyleSheet("color: black");
    isStartRB->setChecked(true);
    isEndRB = new QRadioButton(tr("Конец"));
    isEndRB->setStyleSheet("color: black");
    mainWorkLayout->addWidget(mainWorkPB, 0, 0);
    mainWorkLayout->addWidget(calibrPB, 0, 1);
    mainWorkLayout->addWidget(isStartRB, 1, 0);
    mainWorkLayout->addWidget(isEndRB, 1, 1);
    mainWorkGroupBox->setLayout(mainWorkLayout);

    QGroupBox* measureGroupBox = new QGroupBox(tr("Измерения"));
    measureGroupBox->setStyleSheet("color: blue");
    QGridLayout* measureLayout = new QGridLayout;
    tableViewMeasure = new QTableView;
    tableViewMeasure->setStyleSheet("color: black");
    measureLayout->addWidget(tableViewMeasure, 0, 0);
    measureGroupBox->setLayout(measureLayout);

    QGroupBox* graphGroupBox = new QGroupBox(tr("Графики измерений"));
    graphGroupBox->setStyleSheet("color: blue");
    QGridLayout* graphLayout = new QGridLayout;
    currentDistance = new QLabel("Текущая дальность:");
    currentDistance->setStyleSheet("color: blue");
    currentAzimuth = new QLabel("Текущий азимут:");
    currentAzimuth->setStyleSheet("color: blue");
    currentElevation = new QLabel("Текущий угол места:");
    currentElevation->setStyleSheet("color: blue");
    widgetDistatnce = new QCustomPlot();
    widgetAzimuth = new QCustomPlot();
    widgetElevation = new QCustomPlot();
    graphLayout->addWidget(currentDistance, 0, 0);
    graphLayout->addWidget(widgetDistatnce, 1, 0, 8, 1);
    graphLayout->addWidget(currentAzimuth, 9, 0);
    graphLayout->addWidget(widgetAzimuth, 10, 0, 5, 1);
    graphLayout->addWidget(currentElevation, 15, 0);
    graphLayout->addWidget(widgetElevation, 16, 0, 5, 1);
    graphGroupBox->setLayout(graphLayout);

    mainLayout->addWidget(confGroupBox, 0, 0, 1, 2);
    mainLayout->addWidget(mainWorkGroupBox, 0, 2, 1, 2);
    mainLayout->addWidget(measureGroupBox, 1, 0, 1, 4);
    mainLayout->addWidget(graphGroupBox, 0, 4, 2, 3);
    centralWidget()->setLayout(mainLayout);
}

void MainWindow::mainWorkIsChecked(bool c)
{
    calibrPB->setChecked(!c);
    isStartRB->setEnabled(!c);
    isEndRB->setEnabled(!c);
    if(c) {
        mainWorkPB->setStyleSheet("color: black; background-color: lightgreen;");
        calibrPB->setStyleSheet("color: black; background-color: lightgray;");
    } else {
        mainWorkPB->setStyleSheet("color: black; background-color: lightgray;");
        calibrPB->setStyleSheet("color: black; background-color: lightgreen;");
    }
}

void MainWindow::calibrIsChecked(bool c)
{
    mainWorkPB->setChecked(!c);
    isStartRB->setEnabled(c);
    isEndRB->setEnabled(c);
    if(c) {
        mainWorkPB->setStyleSheet("color: black; background-color: lightgray;");
        calibrPB->setStyleSheet("color: black; background-color: lightgreen;");
    } else {
        mainWorkPB->setStyleSheet("color: black; background-color: lightgreen;");
        calibrPB->setStyleSheet("color: black; background-color: lightgray;");
    }
}

void MainWindow::workNumChanged(QString s)
{
    QSettings* settings = new QSettings("nwmachine", "settings");
    settings->setValue("/main/work_num", s.toInt());
    delete settings;
}

void MainWindow::launchNumChanged(QString s)
{
    QSettings* settings = new QSettings("nwmachine", "settings");
    settings->setValue("/main/launch_num", s);
    delete settings;
}

void MainWindow::isStartToggled(bool c)
{
    QSettings settings("nwmachine", "settings");
    settings.setValue(("/main/isStart"), c);
}

void MainWindow::createStatusBar()
{
    ledRS = new QLabel;
    ledRS->setStyleSheet("image: url(:/images/ledred.png)");
    labelRS = new QLabel("RS-232 готов");
    labelRS->setStyleSheet("color: red;");
    labelRS->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    ledVC = new QLabel;
    ledVC->setStyleSheet("image: url(:/images/ledred.png)");
    labelVC = new QLabel("Есть связь с ВЦ");
    labelVC->setStyleSheet("color: red");
    labelVC->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(ledRS);
    statusBar()->addPermanentWidget(labelRS);
    statusBar()->addPermanentWidget(ledVC);
    statusBar()->addPermanentWidget(labelVC);
}

void MainWindow::createHeader() {

    QStringList horizontalHeader;
    horizontalHeader.append(tr("Время"));
    horizontalHeader.append(tr("Дальн."));
    horizontalHeader.append(tr("КД"));
    horizontalHeader.append(tr("ВКД"));
    horizontalHeader.append(tr("Аз."));
    horizontalHeader.append(tr("Уг.М"));
    horizontalHeader.append(tr("КУ"));
    horizontalHeader.append(tr("АРУ"));
    horizontalHeader.append(tr("РУ"));

    modelHeader->setHorizontalHeaderLabels(horizontalHeader);

    tableViewMeasure->setModel(modelHeader);
    tableViewMeasure->resizeColumnsToContents();
    tableViewMeasure->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
//    tableViewMeasure->horizontalHeader()->setStretchLastSection(true);

    connect(tableViewMeasure->model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), tableViewMeasure, SLOT(scrollToBottom()));
}

void MainWindow::createGraphAxis()
{
    //дальность

    widgetDistatnce->addGraph(); // линия
    widgetDistatnce->graph(0)->setPen(QPen(Qt::blue));
    widgetDistatnce->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    widgetDistatnce->addGraph(); // точка
    widgetDistatnce->graph(1)->setPen(QPen(Qt::blue));
    widgetDistatnce->graph(1)->setLineStyle(QCPGraph::lsNone);
    widgetDistatnce->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);

    widgetDistatnce->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    widgetDistatnce->xAxis->setDateTimeFormat("hh:mm:ss");
    widgetDistatnce->xAxis->setTickStep(1);
    widgetDistatnce->xAxis->setAutoTickStep(false);
    widgetDistatnce->yAxis->setRange(0,3000);
    widgetDistatnce->axisRect()->setupFullAxesBox();

    connect(widgetDistatnce->xAxis, SIGNAL(rangeChanged(QCPRange)), widgetAzimuth->xAxis2, SLOT(setRange(QCPRange)));
    connect(widgetDistatnce->yAxis, SIGNAL(rangeChanged(QCPRange)), widgetAzimuth->yAxis2, SLOT(setRange(QCPRange)));

    widgetAzimuth->addGraph();
    widgetAzimuth->graph(0)->setPen(QPen(Qt::blue));
    widgetAzimuth->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    widgetAzimuth->addGraph();
    widgetAzimuth->graph(1)->setPen(QPen(Qt::blue));
    widgetAzimuth->graph(1)->setLineStyle(QCPGraph::lsNone);
    widgetAzimuth->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);

    widgetAzimuth->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    widgetAzimuth->xAxis->setDateTimeFormat("hh:mm:ss");
    widgetAzimuth->xAxis->setAutoTickStep(false);
    widgetAzimuth->xAxis->setTickStep(1);

    widgetAzimuth->yAxis->setRange(0,360);
    widgetAzimuth->axisRect()->setupFullAxesBox();

    connect(widgetAzimuth->xAxis, SIGNAL(rangeChanged(QCPRange)), widgetAzimuth->xAxis2, SLOT(setRange(QCPRange)));
    connect(widgetAzimuth->yAxis, SIGNAL(rangeChanged(QCPRange)), widgetAzimuth->yAxis2, SLOT(setRange(QCPRange)));

    //угол места

    widgetElevation->addGraph();
    widgetElevation->graph(0)->setPen(QPen(Qt::blue));
    widgetElevation->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    widgetElevation->addGraph();
    widgetElevation->graph(1)->setPen(QPen(Qt::blue));
    widgetElevation->graph(1)->setLineStyle(QCPGraph::lsNone);
    widgetElevation->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);

    widgetElevation->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    widgetElevation->xAxis->setDateTimeFormat("hh:mm:ss");
    widgetElevation->xAxis->setAutoTickStep(false);
    widgetElevation->xAxis->setTickStep(1);
    widgetElevation->yAxis->setRange(-5,90);
    widgetElevation->axisRect()->setupFullAxesBox();

    connect(widgetElevation->xAxis, SIGNAL(rangeChanged(QCPRange)), widgetElevation->xAxis2, SLOT(setRange(QCPRange)));
    connect(widgetElevation->yAxis, SIGNAL(rangeChanged(QCPRange)), widgetElevation->yAxis2, SLOT(setRange(QCPRange)));
}

void MainWindow::addToBuffer(const unsigned char ch)
{
    dataBuffer.append(ch);
}

void MainWindow::viewFile()
{
    disconnect(this, SIGNAL(rx(unsigned char)), this, SLOT(addToBuffer(unsigned char)));
    disconnect(mFrameHandler, SIGNAL(readyToSend(const kama::protocol::Envelope&)), this, SLOT(createGraph(const kama::protocol::Envelope&)));
    disconnect(mFrameHandler,  SIGNAL(startSession()), this, SLOT(handleStartSession()));
    disconnect(mFrameHandler,  SIGNAL(endSession()), this, SLOT(handleEndSession()));
    modelHeader->setRowCount(0);

    QFile file;
    QString path = QDir::home().absolutePath().append("/arch/");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Выбрать файл"), path, tr("Все файлы (*)"));
    if(fileName.isEmpty())
        return;class NetworkDevice;
    class FrameHandler;
    class PointingsUploader;
    class ControlServer;
    struct Frame;
    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QString errStr = QTime::currentTime().toString() + tr(" Ошибка: не могу открыть файл ") + file.fileName() + " . " + file.errorString();
        logWindow->log(errStr);
        statusBar()->showMessage(errStr, 5000);
//        qDebug() << errStr;
        QMessageBox::critical(this, tr("Ошибка"), tr("Не могу открыть файл"));
        return;
    }

    QByteArray array = file.readAll();
    file.close();

    QBuffer buf(&array);

    buf.open(QIODevice::ReadOnly);
    char ch;
    while(buf.getChar(&ch)) {
        emit rx((unsigned char)ch);
    }
    buf.close();
    connect(this, SIGNAL(rx(unsigned char)), SLOT(addToBuffer(unsigned char)));
    connect(mFrameHandler, SIGNAL(readyToSend(const kama::protocol::Envelope&)), SLOT(createGraph(const kama::protocol::Envelope&)));
    connect(mFrameHandler,  SIGNAL(startSession()), SLOT(handleStartSession()));
    connect(mFrameHandler,  SIGNAL(endSession()), SLOT(handleEndSession()));
}

void MainWindow::sendFile(const QString f)
{
    QFile *ifile = new QFile;
    ifile->setFileName(f);
    if(!ifile->exists()) {
        QString errStr = QTime::currentTime().toString() + QString(tr(" Ошибка: Не могу отправить файл с именем ")) +
                ifile->fileName() + ". Файл не существует.";
        logWindow->log(errStr);
        statusBar()->showMessage(errStr, 5000);
        return;
    }
    kama::protocol::DataFile dataFile;
    if(!ifile->open(QIODevice::ReadOnly)) {
        QString errStr = QTime::currentTime().toString() + tr(" Ошибка: Не могу открыть файл ") + ifile->fileName() + " . " + ifile->errorString();
        logWindow->log(errStr);
        statusBar()->showMessage(errStr, 5000);
        return;
    }
    QDataStream istream;
    istream.setDevice(ifile);
    if(!ifile->size())
        return;
    while(!istream.atEnd()) {
        char tmp;
        istream.readRawData(&tmp, 1);
        dataFile.mutable_data_bytes()->append(&tmp, 1);
    }
    QFileInfo fileInfo(ifile->fileName());
    dataFile.set_mp_num(setDialog->ui->mpNumLineEdit->text().toInt());
    dataFile.mutable_file_name()->append(fileInfo.fileName().toStdString());
    if(mainWorkPB->isChecked())
        dataFile.set_main_work(true);
    else
        dataFile.set_main_work(false);

    kama::protocol::Envelope msg;
    msg.mutable_datafile()->CopyFrom(dataFile);
    QString str;
    if(tcpSocket->state() == QAbstractSocket::ConnectedState) {
        str =  QTime::currentTime().toString() + QString(tr(" Файл ")).append(fileInfo.fileName()).append(tr(" передан на ВЦ"));
        tcpSocket->write(serializeDelimited(msg));
    } else {
        str  = QTime::currentTime().toString() + QString(tr( " Нет связи с ВЦ. ")).append(tcpSocket->errorString());
    }
    logWindow->log(str);
    statusBar()->showMessage(str, 5000);
    ifile->close();
    delete ifile;
}

void MainWindow::slotSendFile()
{
    QString path = setDialog->ui->saveDirLineEdit->text();
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Выбрать файл"), path, tr("Все файлы (*)"));
    if(fileNames.isEmpty())
        return;
    foreach (QString x, fileNames) {
        sendFile(x);
    }
}

void MainWindow::handleStartSession()
{
    actionOpen->setEnabled(false);
    workNum->setEnabled(false);
    launchNum->setEnabled(false);
    mainWorkPB->setEnabled(false);
    calibrPB->setEnabled(false);
    isStartRB->setEnabled(false);
    isEndRB->setEnabled(false);
    clearTW();
}

void MainWindow::handleEndSession()
{
    QString path = setDialog->ui->saveDirLineEdit->text();
    QDir archDir = QDir(path);
    if(!archDir.exists()) archDir.mkdir(path);
    QString fileName("");
    if(mainWorkPB->isChecked()) {
        fileName = launchNum->text();
    }
    if(calibrPB->isChecked()) {
        QString suf = isStartRB->isChecked() ? "n" : "k";
        fileName = workNum->text().append(suf);
    }
    fileName = archDir.absoluteFilePath(fileName);
    QFile f(fileName);
    QByteArray tmp = dataBuffer;
    dataBuffer.clear();
    if(f.open(QIODevice::WriteOnly)) {
        int p = tmp.indexOf(F_START);
        tmp = tmp.remove(0, p);
        f.write(tmp);
        f.close();
        sendFile(fileName);
    } else {
        QString errStr = QTime::currentTime().toString() + QString(tr(" Ошибка: Не могу создать файл с именем ")) +
                fileName + ". " + f.errorString();
        logWindow->log(errStr);
        statusBar()->showMessage(errStr, 5000);
    }
    actionOpen->setEnabled(true);
    workNum->setEnabled(true);
    launchNum->setEnabled(true);
    mainWorkPB->setEnabled(true);
    calibrPB->setEnabled(true);
    isStartRB->setEnabled(true);
    isEndRB->setEnabled(true);
    if(mainWorkPB->isChecked()) {
        int num = launchNum->text().toInt();
        launchNum->setText(QString::number(++num));
    }
}

void MainWindow::openSerialPort()
{
    if(serial()->isOpen()) closeSerialPort();

    QString device = setDialog->ui->serialPortInfoListBox->currentText();
    QSerialPort::BaudRate baud = static_cast<QSerialPort::BaudRate>(setDialog->ui->baudRateBox->currentText().toInt());
    QSerialPort::DataBits dbits = static_cast<QSerialPort::DataBits>(setDialog->ui->dataBitsBox->currentText().toInt());
    QSerialPort::StopBits sbits = static_cast<QSerialPort::StopBits>(setDialog->ui->stopBitsBox->currentText().toInt());
    QSerialPort::Parity parity = static_cast<QSerialPort::Parity>(setDialog->ui->parityBox->itemData(setDialog->ui->parityBox->currentIndex()).toInt());
    QSerialPort::FlowControl flow = static_cast<QSerialPort::FlowControl>(setDialog->ui->parityBox->itemData(setDialog->ui->flowControlBox->currentIndex()).toInt());

    serial()->setPortName(device);
    serial()->setBaudRate(baud);
    serial()->setDataBits(dbits);
    serial()->setStopBits(sbits);
    serial()->setParity(parity);
    serial()->setFlowControl(flow);
    if (serial()->open(QIODevice::ReadOnly)) {
        connect(mSerial, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
        ledRS->setStyleSheet("image: url(:/images/ledgreen.png)");
        labelRS->setStyleSheet("color: green;");
        QString str = QTime::currentTime().toString() +
                tr(" Открыт COM-порт %1. Настройки  %1 : %2, %3, %4, %5, %6")
                                                  .arg(device).arg(baud).arg(dbits)
                                                  .arg(parity).arg(sbits).arg(flow);
        logWindow->log(str);
        statusBar()->showMessage(str, 5000);
    } else {
       ledRS->setStyleSheet("image: url(:/images/ledred.png)");
       labelRS->setStyleSheet("color: red;");
       QString str = QTime::currentTime().toString() +
               tr(" Ошибка: не удалось открыть COM-порт %1 ").arg(device) + serial()->errorString();
       logWindow->log(str);
       statusBar()->showMessage(str, 5000);
    }
}
void MainWindow::closeSerialPort()
{
    disconnect(mSerial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    serial()->close();
    ledRS->setStyleSheet("image: url(:/images/ledred.png)");
    labelRS->setStyleSheet("color: red;");
}

void MainWindow::readActivated()
{
    QByteArray array = serial()->readAll();
    QBuffer buf(&array);
    buf.open(QIODevice::ReadOnly);
    int wn = workNum->text().toInt();
    int ln = launchNum->text().toInt();
    bool mw = mainWorkPB->isChecked() ? true : false;
    char ch;
    while(buf.getChar(&ch)) {
        emit rx((unsigned char)ch, wn, ln, mw);
    }
    buf.close();
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Ошибка записи в COM-порт"), serial()->errorString());
        QString errStr = QTime::currentTime().toString() + tr(" Ошибка записи в COM-порт: ") + serial()->errorString();
        logWindow->log(errStr);
        statusBar()->showMessage(errStr, 5000);
//        qDebug() << errStr;
    }
}

void MainWindow::handleError(QString error)
{
    QMessageBox::critical(this, tr("Ошибка!"), error);
    logWindow->log(error);
    statusBar()->showMessage(error, 5000);
//    qDebug() << errStr;
}

void MainWindow::connectToServer()
{
    tcpSocket->abort();
    QString SRV_ADDR = setDialog->ui->destIPLineEdit->text();
    int SRV_PORT = setDialog->ui->destPortLineEdit->text().toInt();

    tcpSocket->connectToHost(SRV_ADDR, SRV_PORT);

    nextBlockSize=0;
}

void MainWindow::slotConnected()
{
    if(timerId != -1) {
        killTimer(timerId);
        timerId = -1;
    }
    ledVC->setStyleSheet("image: url(:/images/ledgreen.png)");
    labelVC->setStyleSheet("color: green;");
    QString str = QTime::currentTime().toString() + tr(" Связь с сервером установлена");
    logWindow->log(str);
    statusBar()->showMessage(str, 5000);
}

void MainWindow::slotDisconnected()
{
    QString str = QTime::currentTime().toString() +
             tr(" Ошибка: ") + QString(tr("Связь с сервером разорвана"));
    ledVC->setStyleSheet("image: url(:/images/ledred.png)");
    labelVC->setStyleSheet("color: red;");
    logWindow->log(str);
}

void MainWindow::slotReadyRead()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_4_3);

    forever {
           if (!nextBlockSize) {
               if (tcpSocket->bytesAvailable() < (int)sizeof(quint16)) {
                   break;
               }
                 in >> nextBlockSize;
           }
           if (tcpSocket->bytesAvailable() < nextBlockSize) {
               break;
           }
        nextBlockSize=0;
    }
}

void MainWindow::sendMessage(const kama::protocol::Envelope& msg)
{
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->write(serializeDelimited(msg));
}

void MainWindow::handleError(QAbstractSocket::SocketError err) {

    ledVC->setStyleSheet("image: url(:/images/ledred.png)");
    labelVC->setStyleSheet("color: red;");
    QString errStr = QTime::currentTime().toString() +
        tr(" Ошибка: ") + (err == QAbstractSocket::HostNotFoundError ?
                         tr("Удаленный узел не найден") :
                         err == QAbstractSocket::RemoteHostClosedError ?
                         tr("Удаленный узел закрыт") :
                         err == QAbstractSocket::ConnectionRefusedError ?
                         tr("Соединение c сервером не установлено") :
                         err == QAbstractSocket::SocketTimeoutError ?
                         tr("Истекло время ожидания соединения") :
                         QString(tcpSocket->errorString())
                        );
//    QString log = ui->logtextEdit->toPlainText();
//    QStringList logList = log.split("\n");
//    QString lastLoggedStr = logList.at(logList.length() - 1);
//    if(errStr != lastLoggedStr) ui->logtextEdit->append(errStr);
    logWindow->log(errStr);
    statusBar()->showMessage(errStr, 5000);
//    qDebug() << errStr;

    if(timerId == -1) {
        timerId = startTimer(30 * 1000);
    }
}

/*virtual*/void MainWindow::timerEvent(QTimerEvent *)
{
    connectToServer();
}

void MainWindow::print(const kama::protocol::Envelope& msg)
{
    if(msg.has_endsession())
        return;
    kama::protocol::Frame frame;
    frame.CopyFrom(msg.frame());
    int row = modelHeader->rowCount();

    modelHeader->setRowCount(row+1);

    QStringList fields;

    DecToDeg az = decToDeg(frame.azimuth());
    DecToDeg el = decToDeg(frame.elevation());
    QString time, cd, azStr, elStr, ca, gc;
    time = QString::fromStdString(frame.time());
    time.chop(1);
    switch (frame.distance_channel_tracking_mode()) {
    case 0:
        cd = QString(tr("РУ"));
        break;
    case 1:
        cd = QString(tr("АС грубое"));
        break;
    case 2:
        cd = QString(tr("по памяти"));
        break;
    case 3:
        cd = QString(tr("АС точное"));
        break;
    default:
        cd = QString("");
        break;
    }
    switch (frame.angle_channel_tracking_mode()) {
    case 0:
        ca = QString(tr("РУ"));
        break;
    case 1:
        ca = QString(tr("РУ ")).append(QChar(949));
        break;
    case 2:
        ca = QString(tr("АС"));
        break;
    case 4:
        ca = QString(tr("ЦУк"));
        break;
    case 5:
        ca = QString(tr("ПН"));
        break;
    default:
        ca = QString::number(frame.angle_channel_tracking_mode());
        break;
    }
    switch (frame.gain_gontrol()) {
    case 0:
        gc = QString(tr("РРУ"));
        break;
    case 1:
        gc = QString(tr("АРУ"));
        break;
    default:
        gc = QString("");
        break;
    }
    azStr.append(QString::number(az.deg)).append(QChar(176)).append(QString::number(az.min)).append("'").append(QString::number(az.sec)).append("''");
    elStr.append(QString::number(el.deg)).append(QChar(176)).append(QString::number(el.min)).append("'").append(QString::number(el.sec)).append("''");
    fields
            << time
            << QString::number(frame.distance())
            << cd
            << QString::number(frame.distance_is_valid())
            << azStr
            << elStr
            << ca
            << QString::number(frame.agc_level())
            << gc;

     for (int i = 0; i < fields.count(); ++i) {
         modelHeader->setItem(row, i,
                              new QStandardItem(fields[i]));
         modelHeader->item(row,i)->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsSelectable);
    }
    tableViewMeasure->resizeColumnsToContents();
}

void MainWindow::createGraph(const kama::protocol::Envelope& msg)
{
    kama::protocol::Frame frame;
    frame.CopyFrom(msg.frame());

    const std::string& TimeMeasure = frame.time();

    QString ttime;
    double t, p, b, e;

    ttime = QString::fromStdString(TimeMeasure);

    t= QDateTime::fromString(ttime, "hh:mm:ss.zzz").toMSecsSinceEpoch()/1000.0;

    p = QString::number(frame.distance()).toDouble();
    b = QString::number(frame.azimuth()).toDouble();
    e = QString::number(frame.elevation()).toDouble();

    DecToDeg az = decToDeg(frame.azimuth());
    DecToDeg el = decToDeg(frame.elevation());
    QString azStr, elStr;
    azStr.append(QString::number(az.deg)).append(QChar(176)).append(QString::number(az.min)).append("'").append(QString::number(az.sec)).append("''");
    elStr.append(QString::number(el.deg)).append(QChar(176)).append(QString::number(el.min)).append("'").append(QString::number(el.sec)).append("''");
    currentDistance->setText("Текущая дальность: " + QString::number(frame.distance()) +" м");
    currentAzimuth->setText("Текущий азимут: " + azStr);
    currentElevation->setText("Текущий угол места: " + elStr);

    //дальность

    widgetDistatnce->graph(0)->addData(t, p);

    // данные для точки
    widgetDistatnce->graph(1)->clearData();
    widgetDistatnce->graph(1)->addData(t, p);

    // удалить данные за пределами диапазона
    widgetDistatnce->graph(0)->removeDataBefore(t-4);

    // масштабирование графика под текущие измерения

    widgetDistatnce->graph(0)->rescaleValueAxis(true);
    widgetDistatnce->graph(1)->rescaleValueAxis(true);
    widgetDistatnce->yAxis->rescale(true);
    widgetDistatnce->yAxis->setRange(p-p/3,p+p/2);
    widgetDistatnce->xAxis->setRange(t+0.25, 4, Qt::AlignRight);
    widgetDistatnce->replot();

    //азимут

    widgetAzimuth->graph(0)->addData(t, b);

    widgetAzimuth->graph(1)->clearData();
    widgetAzimuth->graph(1)->addData(t, b);

    widgetAzimuth->graph(0)->removeDataBefore(t-4);

    widgetAzimuth->graph(0)->rescaleValueAxis(true);
    widgetAzimuth->graph(1)->rescaleValueAxis(true);
    widgetAzimuth->yAxis->rescale(true);
    widgetAzimuth->yAxis->setRange(b-b/2,b+b/2);
    widgetAzimuth->xAxis->setRange(t+0.25, 4, Qt::AlignRight);
    widgetAzimuth->replot();

    // угол места

    widgetElevation->graph(0)->addData(t, e);

    widgetElevation->graph(1)->clearData();
    widgetElevation->graph(1)->addData(t, e);

    widgetElevation->graph(0)->removeDataBefore(t-4);

    widgetElevation->graph(0)->rescaleValueAxis(true);
    widgetElevation->graph(1)->rescaleValueAxis(true);
    widgetElevation->yAxis->rescale(true);
    widgetElevation->yAxis->setRange(e-e/2,e+e/2);
    widgetElevation->xAxis->setRange(t+0.25 , 4, Qt::AlignRight);
    widgetElevation->replot();
}

void MainWindow::clearTW()
{
    modelHeader->clear();
    createHeader();
}

void MainWindow::about()
{
    QString info = QString("<p><h3>Программное обеспечение блока КНВ-010 РЛС \"Кама-Н\"</h3></p> \
                            <p><h5>CЮИТ.00130-01</h5></p> \
                            <p><h5>АО \"КБ\"Кунцево\", 2015</h5></p>");
    QMessageBox::about(this, "О программе", info);
}

void MainWindow::helpDesk()
{
    QString info = QString("<p><h3>За технической поддержкой Вы можете обратиться:</h3></p> \
                            <p><h5>\"КБ \"Кунцево\", Клевчиков Алексей Николаевич</h5></p> \
                            <p><h5>тел.: +7(499)557-08-94</h5></p> \
                            <p><h5>email: klevchikov@mail.ru");
    QMessageBox::about(this, "О программе", info);
}

void MainWindow::help()
{

}

bool MainWindow::askOnClose()
{
    int r = QMessageBox::question(
        this, tr("Подтвердите"),
        tr("Выйти из программы?"),
        QMessageBox::No | QMessageBox::Yes);
    return (r == QMessageBox::Yes);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (askOnClose()) {
        event->accept();
    } else {
        event->ignore();
    }
}
