#include "polluxusmain.h"
#include <QMouseEvent>

#include <QDesktopWidget>
#include <QDebug>
#include "posixibclient.h"
#include "messageprocessor.h"
#include "polluxuslogger.h"
#include "digitalclock.h"
#include "marketdata.h"
#include "contractmanager.h"
#include "orderbookwidget.h"

PolluxusMain::PolluxusMain(QWidget *parent) :
    QWidget(parent, Qt::FramelessWindowHint)
{

    m_nMouseClick_X_Coordinate = 0;
    m_nMouseClick_Y_Coordinate = 0;

    loadGateway();

    QLabel *pLogo = new QLabel();
    pLogo->setFixedWidth(24);
    pLogo->setFixedHeight(24);
    pLogo->setScaledContents( true );
    pLogo->setPixmap(QPixmap(":/images/setup.png"));

    createMenuBar();
    createToolBar();

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setSpacing(0);
    hLayout->setMargin(0);
    hLayout->setContentsMargins(0,0,0,0);

    hLayout->addWidget(pLogo);
    hLayout->addWidget(pMenuBar);
    hLayout->addWidget(spacer);
    hLayout->addWidget(pToolBar);
    setLayout(hLayout);

    adjustTopBarPosition();


    pIBAdapter = new PosixIBClient;

    std::shared_ptr<PosixIBClient> pIBClient(pIBAdapter);
    pMsgProcessor = new MessageProcessor(pIBClient);

    connect(pIBAdapter, SIGNAL(AdapterConnected()), this, SLOT(onAdapterConnected()));
    connect(pIBAdapter, SIGNAL(AdapterDisconnected()), this, SLOT(onAdapterDisconnected()));
    connect(pIBAdapter, SIGNAL(AdjustTimeDiff(qint64)), this, SLOT(onAdjustTimeDiff(qint64)));

    pLogger = new PolluxusLogger(this);
    pLogger->show();

    pContractManager = new ContractManager(this);
    pContractManager->show();


    connect(pIBAdapter, SIGNAL(OrderUpdated(QString)), pLogger, SLOT(onOrderUpdated(QString)));
    connect(pIBAdapter, SIGNAL(TickUpdating(const Tick)), pContractManager, SLOT(onTickUpdating(const Tick)));

    loadWorkSpace();


    qRegisterMetaType<Tick>("Tick");
    qRegisterMetaType<Tick>("Tick&");
    qRegisterMetaType<Depth>("Depth");
    qRegisterMetaType<Depth>("Depth&");
    qRegisterMetaType<TickerData>("TickerData");
    qRegisterMetaType<TickerData>("TickerData&");

    qDebug()<<this->rect().topLeft();
}

PolluxusMain::~PolluxusMain()
{

    saveWorkSpace();
}


void PolluxusMain::mousePressEvent(QMouseEvent* event)
{
    //qDebug() << "mousePressEvent()";
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}

void PolluxusMain::mouseMoveEvent(QMouseEvent* event)
{
    //qDebug() << "mouseMoveEvent()";
    move(event->globalX()-m_nMouseClick_X_Coordinate,event->globalY()-m_nMouseClick_Y_Coordinate);
}

void PolluxusMain::mouseReleaseEvent(QMouseEvent* event)
{
    //qDebug() << "mouseReleaseEvent()";
    if(event->globalY() < 100)
    {
        //move(-(event->globalX()),-(event->globalY()));
        //move(-50, -50);

        qDebug()<<this->rect().topLeft();
    }
}

void PolluxusMain::createMenuBar()
{
    pMenuBar = new QMenuBar;

    QAction *quit = new QAction("&Quit", this);
    QAction *saveWS = new QAction("&Save Workspace", this);

//    QMenu *logo;
//    logo = pMenuBar->addMenu(QIcon(":/images/setup.png"), "Polluxus");

    QMenu *file;
    file = pMenuBar->addMenu("&File");
    file->addAction(quit);
    file->addAction(saveWS);

    QAction *viewCM = new QAction("Show ContractManager", this);
    QAction *viewLogger = new QAction("Show Logger", this);

    QMenu *view;
    view = pMenuBar->addMenu("&View");
    view->addAction(viewCM);
    view->addAction(viewLogger);


    QMenu *trading;
    trading = pMenuBar->addMenu("&Trading");


    QMenu *about;
    about = pMenuBar->addMenu("&About");


    pMenuBar->addSeparator();

    connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(saveWS, SIGNAL(triggered()), this, SLOT(onSaveWorkSpaces()));
    connect(viewCM,SIGNAL(triggered()), this, SLOT(onViewContractManager()));
    connect(viewLogger,SIGNAL(triggered()), this, SLOT(onViewLogger()));

    pMenuBar->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

void PolluxusMain::createToolBar()
{

    pToolBar = new QToolBar;


    btnNewOrderBookWidget = new QPushButton("New OrderBook");
    btnTest = new QPushButton(tr("test"));

    btnConnect = new QPushButton(tr("Connect"));
    btnConnect->setCheckable(true);


    pClock = new DigitalClock(this);



    lbLight = new QLabel();
    lbLight->setFixedWidth(24);
    lbLight->setFixedHeight(24);
    lbLight->setScaledContents( true );
    lbLight->setAlignment(Qt::AlignRight);
    lbLight->setPixmap(QPixmap(":/images/bullet-red.png"));

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    pToolBar->addWidget(spacer);

    pToolBar->addWidget(btnNewOrderBookWidget);
    pToolBar->addWidget(btnTest);
    pToolBar->addWidget(btnConnect);

    pToolBar->addWidget(pClock);
    pToolBar->addWidget(lbLight);

    //pClock->show();

    pToolBar->adjustSize();

    pToolBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    connect(btnNewOrderBookWidget, SIGNAL(clicked(bool)), this, SLOT(onNewOrderBookWidget()), Qt::DirectConnection);
    connect(btnTest, SIGNAL(clicked(bool)), this, SLOT(onTest()), Qt::DirectConnection);
    connect(btnConnect, SIGNAL(toggled(bool)), this, SLOT(onAdapterConnect()), Qt::DirectConnection);

}

void PolluxusMain::adjustTopBarPosition()
{
    resize(QDesktopWidget().availableGeometry().width(), 24);
    move(0, 0);
    this->setFixedSize(this->size());
}



void PolluxusMain::onTest()
{
    QMetaObject::invokeMethod(pIBAdapter, "onTest", Qt::QueuedConnection);

}

void PolluxusMain::onAdjustTimeDiff(qint64 timeDiffMS)
{
    pClock->setTimeDiffMS(timeDiffMS);
    qDebug() << "Signal AdjustTimeDiff() received";
}

void PolluxusMain::onAdapterConnect()
{
    if(btnConnect->isChecked())
    {
        qDebug() << "MainWindow:Hi I am connecting IB.------"  << QThread::currentThreadId();


        QMetaObject::invokeMethod(pIBAdapter, "onConnect", Qt::QueuedConnection,
                                  Q_ARG(QString, host),
                                  Q_ARG(int, port),
                                  Q_ARG(int, clientId));

        btnConnect->setText(tr("Connecting..."));
        btnConnect->setEnabled(false);


        lbLight->setPixmap(QPixmap(":/images/wait.png"));

    }

    else
    {
        qDebug() << "MainWindow:Hi I am disconnecting ib.------"  << QThread::currentThreadId();

        QMetaObject::invokeMethod(pIBAdapter, "onDisconnect", Qt::QueuedConnection );

        btnConnect->setText(tr("Disconnecting"));
        btnConnect->setEnabled(false);
        lbLight->setPixmap(QPixmap(":/images/bullet-grey.png"));

    }
}

void PolluxusMain::onAdapterConnected()
{
    qDebug() << "MainWindow:Recv connected signal from Posix.------"  << QThread::currentThreadId();

    btnConnect->setText(tr("Disconnect"));
    btnConnect->setEnabled(true);


    lbLight->setPixmap(QPixmap(":/images/bullet-green.png"));

    pMsgProcessor->start();

    //QMetaObject::invokeMethod(pIBAdapter, "onReqCurrentTime", Qt::QueuedConnection);
}

void PolluxusMain::onAdapterDisconnected()
{
    qDebug() << "MainWindow:Recv disconnected signal from Posix.------"  << QThread::currentThreadId();

    btnConnect->setText(tr("Connect"));
    btnConnect->setEnabled(true);
    lbLight->setPixmap(QPixmap(":/images/bullet-red.png"));

}


void PolluxusMain::saveWorkSpace()
{
    qDebug() << "PolluxusMain::saveWorkSpace";
    QString iniFileString = QDir::currentPath() + "/workspace.ini";
    QSettings *wsSettings = new QSettings(iniFileString, QSettings::IniFormat);

    wsSettings->setValue("appname", "Polluxus");

    wsSettings->beginGroup("PolluxusMain");
    wsSettings->setValue("geometry", saveGeometry());
    wsSettings->setValue( "maximized", isMaximized());
    if ( !isMaximized() ) {
            wsSettings->setValue( "pos", pos() );
            wsSettings->setValue( "size", size() );
        }
    wsSettings->endGroup();
    wsSettings->sync();
}

void PolluxusMain::loadGateway()
{
    qDebug() << "PolluxusMain::loadGateway";
    QString iniFileString = QDir::currentPath() + "/workspace.ini";

    QSettings *wsSettings = new QSettings(iniFileString, QSettings::IniFormat);

    wsSettings->beginGroup("Gateway");
    host = wsSettings->value( "host", "127.0.0.1").toString();
    port = wsSettings->value( "port", 4001).toInt();
    clientId = wsSettings->value( "clientId", 1).toInt();
    wsSettings->endGroup();
}

void PolluxusMain::loadWorkSpace()
{
    qDebug() << "PolluxusMain::loadWorkSpace";
    QString iniFileString = QDir::currentPath() + "/workspace.ini";

    QSettings *wsSettings = new QSettings(iniFileString, QSettings::IniFormat);

    wsSettings->beginGroup("PolluxusMain");
    restoreGeometry(wsSettings->value( "geometry", saveGeometry() ).toByteArray());
    move(wsSettings->value( "pos", pos() ).toPoint());
    resize(wsSettings->value( "size", size()).toSize());
    if ( wsSettings->value( "maximized", isMaximized() ).toBool() )
    {
        showMaximized();
    }
    wsSettings->endGroup();
}

void PolluxusMain::onSaveWorkSpaces()
{
    this->saveWorkSpace();
    pLogger->saveWorkSpace();
    pContractManager->saveWorkSpace();
}

void PolluxusMain::onNewOrderBookWidget()
{
    pContractManager->loadWorkSpace();
//    qDebug() << "onNewOrderBookWidget()";
//    OrderBookWidget *obWidget = new OrderBookWidget(this);
//    obWidget->show();
}

void PolluxusMain::onViewContractManager()
{
    if(!pContractManager)
    {
        pContractManager = new ContractManager(this);
        connect(pIBAdapter, SIGNAL(TickUpdating(const Tick)), pContractManager, SLOT(onTickUpdating(const Tick)));
        pContractManager->show();
    }
    else
    {
        if (!pContractManager->isVisible()) pContractManager->show();
    }
}

void PolluxusMain::onViewLogger()
{
    if(!pLogger)
    {
        pLogger = new PolluxusLogger(this);
        connect(pIBAdapter, SIGNAL(OrderUpdated(QString)), pLogger, SLOT(onOrderUpdated(QString)));
        pLogger->show();
    }
    else
    {
        if (!pLogger->isVisible()) pLogger->show();
    }
}
