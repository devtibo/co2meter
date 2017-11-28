#include "mainwindow.h"
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QCategoryAxis>

#define DEBUG

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    /* Configue windows */
    this->setAutoFillBackground(true);
    QPalette p;
    p.setBrush(QPalette::Window,QBrush(QColor(46,46,46)));
    this->setPalette(p);
    setWindowTitle("CO2 Meter Level v1.0 (copyright TiBo, 2017)");


    /* Configure USB0 UART Connection*/
    const QString portName = QString("/dev/ttyUSB0") ;
    serial = new QSerialPort(this);
    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    /* Open serial connection*/
    if ( ! serial->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
#ifndef DEBUG
        this->close();

#else
        connect(this, SIGNAL(debugSig()), this, SLOT(readData()));
#endif
    }


    /* Configure chart */
    chart = new QCustomPlot();
    chart->addGraph();
    chart->yAxis->setRange(0,3000);
    // configure bottom axis to show date instead of number:
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("hh:mm:ss");
    chart->xAxis->setTicker(dateTicker);
    now = QDateTime::currentDateTime().toTime_t();
    chart->xAxis->setRange(now+refreshRate, now+refreshRate*ticksCount);
    chart->yAxis->setLabel("CO2 level (in ppm)");
    chart->xAxis->setLabel("Clock");
    chart->setBackground(QBrush(QColor(46,46,46)));
    // Customize plot area background
    QLinearGradient plotAreaGradient;
    plotAreaGradient.setStart(QPointF(0, 0));
    plotAreaGradient.setFinalStop(QPointF(0, 1));
    plotAreaGradient.setColorAt(1, QColor::fromRgbF(0, 1, 0, 0.7));
    plotAreaGradient.setColorAt(0.6, QColor::fromRgbF(1, 1, 0, 0.7));
    plotAreaGradient.setColorAt(0.3, QColor::fromRgbF(1, 1, 0, 0.7));
    plotAreaGradient.setColorAt(0, QColor::fromRgbF(1,0 , 0, 0.7));
    plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    chart->axisRect()->setBackground(QBrush(plotAreaGradient));
    // Axis fonts
    chart->xAxis->setLabelColor(Qt::white);
    chart->xAxis->setTickLabelColor(Qt::white);
    chart->yAxis->setLabelColor(Qt::white);
    chart->yAxis->setTickLabelColor(Qt::white);
    /*Add Text to Graph*/
    QFont mfont;
    mfont.setItalic(true);
    mfont.setPixelSize(14);
    groupTracerTextGood = new QCPItemText(chart);
    groupTracerTextGood->setPositionAlignment(Qt::AlignCenter);
    groupTracerTextGood->position->setCoords(now+refreshRate*ticksCount/2, 500); // lower right corner of axis rect
    groupTracerTextGood->setText("Good Air Quality Zone");
    groupTracerTextGood->setFont(mfont);
    groupTracerTextCorrect = new QCPItemText(chart);
    groupTracerTextCorrect->setPositionAlignment(Qt::AlignCenter);
    groupTracerTextCorrect->position->setCoords(now+refreshRate*ticksCount/2, 1500); // lower right corner of axis rect
    groupTracerTextCorrect->setText("Correct Air Quality Zone");
    groupTracerTextCorrect->setFont(mfont);
    groupTracerTextBad = new QCPItemText(chart);
    groupTracerTextBad->setPositionAlignment(Qt::AlignCenter);
    groupTracerTextBad->position->setCoords(now+refreshRate*ticksCount/2, 2500); // lower right corner of axis rect
    groupTracerTextBad->setText("Bad Air Quality Zone");
    groupTracerTextBad->setFont(mfont);

    // Chart connections
    connect(chart, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(onMousePress(QMouseEvent*)));


    /* Phase Tracer */
    // add label for phase tracer:
    phaseTracerText = new QCPItemText(chart);
    phaseTracerText->setPositionAlignment(Qt::AlignLeft|Qt::AlignBottom);
    phaseTracerText->setTextAlignment(Qt::AlignLeft);
    QFont t;
    t.setPixelSize(12);
    t.setBold(true);
    phaseTracerText->setFont(t);
    phaseTracerText->setPadding(QMargins(10, 0, 10, 0));
    phaseTracerText->setBrush(QColor::fromRgbF(0.5,0.5,0.5,0.7));
    // Phase tracer
    phaseTracer = new QCPItemTracer(chart);
    phaseTracer->setGraph(chart->graph(0));
    phaseTracer->setInterpolating(false); // To exactly match the graph data
    phaseTracer->setStyle(QCPItemTracer::tsCircle);
    phaseTracer->setPen(QPen(Qt::red));
    phaseTracer->setBrush(Qt::red);
    phaseTracer->setSize(7);
    phaseTracerText->position->setParentAnchor(phaseTracer->position); // lower right corner of axis rect



    /* OverLevel Reach*/
    // Value label
    co2OverLevelLabel = new QLabel("Level > 2000 ppm reached at least on time");
    t.setPixelSize(20);
    co2OverLevelLabel->setFont(t);
    co2OverLevelLabel->setMaximumHeight(20);
    co2OverLevelLabel->setStyleSheet("QLabel {color : white; }");
    // Led
    mLed = new QLedIndicator(this);
    mLed->setCheckable(false);
    mLed->setOnColor1(Qt::red);
    mLed->setOnColor2(Qt::red);
    mLed->setOffColor1(Qt::gray);
    mLed->setOffColor2(Qt::gray);
    // reset Button
    resetBP = new QPushButton("Reset");
    connect(resetBP,SIGNAL(pressed()), this, SLOT(resetLed()));
    // Set not visible
    co2OverLevelLabel->setVisible(false);
    mLed->setVisible(false);
    resetBP->setVisible(false);
    /* Create Tray*/
    createActions();
    createTrayIcon();
    QIcon icon = QIcon(":/icons/icon_1_6.png");
    trayIcon->setIcon(icon);
    trayIcon->show();

    /*Create and fill layout*/
    QGridLayout *m_layout = new QGridLayout();
    m_layout->addWidget(co2OverLevelLabel,0,1,1,1);
    m_layout->addWidget(resetBP,0,2,1,1,Qt::AlignRight);
    m_layout->addWidget(mLed,0,0,1,1,Qt::AlignRight);
    m_layout->addWidget(chart,1,0,1,3);
    QWidget *c = new QWidget;
    c->setMinimumWidth(600);
    c->setMinimumHeight(400);
    c->setLayout(m_layout);
    setCentralWidget(c);

    /* Create Timer to periodically ask and read co2 value*/
    timer = new QTimer();
    timer->setInterval(refreshRate *1000);
    connect(timer,SIGNAL(timeout()),this,SLOT(onUpdate()));
    timer->start();
}

MainWindow::~MainWindow(){}

void MainWindow::readData()
{   
#ifndef DEBUG
    //Return if response is not complete (i.e. 7 bytes in this case)
    if (serial->bytesAvailable() <7)
        return;

    QByteArray data = serial->readAll(); // Read response
    unsigned int co2Value = ((int)data.at(3) * 256 + (int)data.at(4)); // Convert to CO2 ppm
#else
    unsigned int co2Value = 1000;
#endif

    // Fill buffer
    now = QDateTime::currentDateTime().toTime_t();
    if (timeData.size() < ticksCount) // fill to reach maxsize
    {
        timeData.append(QCPGraphData(now, co2Value));
        // Set tracer on the last point
        setTracerAt(now);
        // rever tracer text after the middle of the graph
        if (phaseTracer->position->key()>chart->xAxis->range().center())
            phaseTracerText->setPositionAlignment(Qt::AlignRight|Qt::AlignBottom);

    }
    else // When totally filled
    {
        // left shift and add at last points
        for (int i=0; i< ticksCount-1;i++)
            timeData.replace(i,timeData.at(i+1));
        // At current point at the end
        timeData.replace(ticksCount-1,QCPGraphData(now, co2Value) );

        // when graph completely filled update xAxis according to date range
        chart->xAxis->rescale();

        // Update Graph Text Position
        groupTracerTextGood->position->setCoords(chart->xAxis->range().center(), 500);
        groupTracerTextCorrect->position->setCoords(chart->xAxis->range().center(), 1500);
        groupTracerTextBad->position->setCoords(chart->xAxis->range().center(), 2500);

        // Let the phaseTracer at the same position on the graph
        int current_idx = -1;
        for (int i=0; i < timeData.size(); i++)
            if ( (timeData.at(i).key) == phaseTracer->position->key())
                current_idx = i;
        setTracerAt(timeData.at(current_idx+1).key);

    }

    // Update chart values
    chart->graph()->data()->set(timeData);
    // Update Graph
    chart->replot();


    /* Update Tray icon and OverLevel indicator*/
    QIcon icon;
    if (co2Value <= 500)
    {
        if (overLevel)
            icon = QIcon(":/icons/icon_1_6OVER.png");
        else
            icon = QIcon(":/icons/icon_1_6.png");
    }
    else if (co2Value <= 1000)
    {
        if (overLevel)
            icon = QIcon(":/icons/icon_2_6OVER.png");
        else
            icon = QIcon(":/icons/icon_2_6.png");
    }
    else if (co2Value <= 1500)
    {
        if (overLevel)
            icon = QIcon(":/icons/icon_3_6OVER.png");
        else
            icon = QIcon(":/icons/icon_3_6.png");
    }
    else if (co2Value <= 2000)
    {
        if (overLevel)
            icon = QIcon(":/icons/icon_4_6OVER.png");
        else
            icon = QIcon(":/icons/icon_4_6.png");
    }
    else if (co2Value <= 2500)
    {
        overLevel = true;
        if (overLevel)
            icon = QIcon(":/icons/icon_5_6OVER.png");
        else
            icon = QIcon(":/icons/icon_5_6.png");
        co2OverLevelLabel->setVisible(true);
        mLed->setVisible(true);
        resetBP->setVisible(true);
        mLed->setStatus(true);
        co2OverLevelLabel->setStyleSheet("QLabel {color : red; }");
        showMessage();
    }
    else if (co2Value > 2500)
    {
        overLevel = true;
        icon = QIcon(":/icons/icon_6_6OVER.png");
        co2OverLevelLabel->setVisible(true);
        mLed->setVisible(true);
        resetBP->setVisible(true);
        mLed->setStatus(true);
        co2OverLevelLabel->setStyleSheet("QLabel {color : red; }");
        showMessage();
    }

    trayIcon->setIcon(icon);
    trayIcon->setToolTip(QString::number(co2Value) + " ppm"); // Not working On Linux. On windows ?
}

// Not close but only hide
void MainWindow::closeEvent(QCloseEvent *event)
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}

// Message display when level is too high
void MainWindow::showMessage()
{
    if (!message_alreadyActive)
    {
        message_alreadyActive = true;
        trayIcon->showMessage("C02 Level", "Critical Level reached !", QSystemTrayIcon::Warning,
                              10 * 1000);
      // A trick to only view message one per one
        QTimer *mTimer = new QTimer;
        mTimer->setSingleShot(true);
        mTimer->setInterval(10*1000);
        connect(mTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
        mTimer->start();
    }
}
// Allow to re-diplsay a message
void MainWindow::onTimer()
{
    message_alreadyActive = false;
}

void MainWindow::exitApp()
{
    if (serial->isOpen())
        serial->close();

    qApp->quit();
}

// Ask for values
void MainWindow::onUpdate()
{
    /* Send Trame for asking CO2 value*/
    QByteArray data;
    data.append((char)0xFE);
    data.append((char)0x44);
    data.append((char)0x00);
    data.append((char)0x08);
    data.append((char)0x02);
    data.append((char)0x9F);
    data.append((char)0x25);
#ifndef DEBUG
    serial->write(data);
#else
    emit debugSig(); // simulate a response
#endif
}


/* Catch mouse clicking to mouve PhaseTracer*/
void MainWindow::onMousePress(QMouseEvent* event)
{
    if (event->buttons()==Qt::LeftButton)
    {
        // Convert to graph coordinates
        double x,y;
        chart->graph(0)->pixelsToCoords(event->pos().x(),event->pos().y(),x,y); // get mouse cursor

        // choose the orientation of the phasetracer text according to the position o,n the graph
        if (x < chart->xAxis->range().center())
            phaseTracerText->setPositionAlignment(Qt::AlignLeft|Qt::AlignBottom);
        else
            phaseTracerText->setPositionAlignment(Qt::AlignRight|Qt::AlignBottom);

        // Update Tracer position
        setTracerAt(x);
    }

    // Update graph
    chart->replot();
}

/* Method to place the phase tracer*/
void MainWindow::setTracerAt(double x)
{
    // Update Position
    phaseTracer->setGraphKey(x);
    phaseTracer->updatePosition(); // else what the position values are not correct

    // Update Text
    QDateTime *tt = new QDateTime;
    tt->setSecsSinceEpoch((qint64)phaseTracer->position->key());
    phaseTracerText->setText(QString("(clock: %1:%2:%3, co2: %4)").arg(tt->time().hour()).arg(tt->time().minute()).arg(tt->time().second()).arg(phaseTracer->position->value()));
}


/* Tray contextMenu*/
void MainWindow::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));
    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));
    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(exitApp()));
}


void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

/* reset over level indicator*/
void MainWindow::resetLed()
{
    mLed->setStatus(false);
    co2OverLevelLabel->setVisible(false);
    mLed->setVisible(false);
    resetBP->setVisible(false);
    overLevel = false;
}
