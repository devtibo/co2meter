#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QTime>
#include <QDateTimeAxis>
#include "qcustomplot/qcustomplot.h"
#include "qledindicator.h"


QT_CHARTS_USE_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QSerialPort *serial ;
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QTimer *timer;

    double now;

    QCPItemText *groupTracerTextGood;
    QCPItemText *groupTracerTextCorrect;
    QCPItemText *groupTracerTextBad;

    QCPItemTracer *phaseTracer;
    QCPItemText *phaseTracerText ;

    int ticksCount = 30;
    int refreshRate = 1;

    QCustomPlot *chart ;
    QFont labelsFont;
    QBrush axisBrush;
    QDateTime t;

    bool message_alreadyActive=false;

    QVector<QCPGraphData> timeData;

    void setTracerAt(double x);
    void createTrayIcon();
    void createActions();
    void showMessage();

    QLedIndicator *mLed;
    QPushButton *resetBP;

    QMenu *trayIconMenu;
    QSystemTrayIcon *trayIcon;
    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;

    QLabel *co2OverLevelLabel;

    bool overLevel = false;

    void closeEvent(QCloseEvent*);

signals:
    void debugSig();

public slots:
    void readData();
    void onUpdate();
    void onMousePress(QMouseEvent*);
    void onTimer();
    void resetLed();
    void exitApp();
};

#endif // MAINWINDOW_H
