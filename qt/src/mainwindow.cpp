#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include "mainwindow.h"
#include "socket_client.h"
#include "config.h"

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif


static MainWindow *mainwindow;


/* QT: main window layout - �����沼�� */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	QImage image;

	/* set window title - ���ô��ڱ��� */
	setWindowTitle(codec->toUnicode(DEFAULT_WINDOW_TITLE));

	/* set window size - ���ô��ڴ�С */
	resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

	/* set central widget - �������ؼ� */
	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	/* set clock widget - ����ʱ�ӿؼ� */
	clockLabel = new QLabel(mainWindow);
	clockLabel->setWordWrap(true);	// adapt to text, can show multi row
	clockLabel->setGeometry(10, 10, 200, 20);	// set geometry - ���ü��δ�С����
	clockLabel->show();	// display widget - ��ʾ����

	/* create timer to show main window - ������ʱ������ʾ������ */
	timer = new QTimer(this);
	/* if timer timeout it will call slot function - �����ʱ����ʱʱ�䵽�˾ͻ���òۺ���showMainwindow() */
	connect(timer, SIGNAL(timeout()), this, SLOT(showMainwindow()));
	/* set timeout time and start timer - ���ó�ʱʱ�䲢������ʱ����TIMER_INTERV_MS=1ms */
	timer->start(TIMER_INTERV_MS);

	/* input (ip:port) widget - ����ؼ���IP��ַ���˿ڣ� */
	ipPortEdit = new QLineEdit(mainWindow);
    ipPortEdit->setPlaceholderText(codec->toUnicode(TEXT_IP_PORT));
    ipPortEdit->setAlignment(Qt::AlignCenter);
	ipPortEdit->setGeometry(10, 100, 150, 30);

	connectBtn = new QPushButton(mainWindow);	// connect/disconnect button - ����/�Ͽ���ť
	connectBtn->setText(codec->toUnicode(TEXT_CONNECT));
	connectBtn->setGeometry(55, 140, 45, 25);
	/* button slot function - �����Ĳۺ������ź���ۣ��������˰���ʱ������õ��ۺ��� */
	connect(connectBtn, SIGNAL(clicked()), this, SLOT(connect_tcp_server()));
	connectBtn->setFocus();

	/* user information image - �û���ϢͼƬ */
	image.load(USER_INFO_IMG);
	userInfo = new QLabel(mainWindow);
	userInfo->setPixmap(QPixmap::fromImage(image));
	userInfo->setGeometry(0, DEFAULT_WINDOW_HEIGHT/2, image.width(), image.height());
	userInfo->show();

	connect_state = 0;
}


MainWindow::~MainWindow(void)
{
	
}

/* timer timeout slot function, to show main window - ��ʱ����ʱʱ�䵽�Ĳۺ�������ʾ������ */
void MainWindow::showMainwindow(void)
{

	timer->stop();

	/* update clock - ����ʱ����ʾ */
	QDateTime time = QDateTime::currentDateTime();
	QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
	clockLabel->setText(str);


	/* start timer again - �ٴ�������ʱ�� */
	timer->start(TIMER_INTERV_MS);

}

/* connect server  - ���ӷ����� */
void MainWindow::connect_tcp_server(void)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	QString ipPort_qstr;
	QByteArray ba;
	char ip_str[32];
	int ret;
	
	if(connect_state == 0)	// disconnected, connect it
	{
		/* get input content - ��ȡ��������� */
		ipPort_qstr = ipPortEdit->text();
		if(ipPort_qstr.length() <9 || ipPort_qstr.length() >20)
		{
			cout << "IP input error!\n" << endl;
			return ;
		}

		qDebug() << ipPort_qstr;
		
		ba = ipPort_qstr.toLatin1();
		memset(ip_str, 0, sizeof(ip_str));
		strncpy((char *)ip_str, ba.data(), strlen(ba.data()));

		/* start client socket to connect server - �����ͻ���socket���ӷ����� */
		ret = socket_client_start(ip_str);
		if(ret == 0)
		{
			ipPortEdit->setEnabled(false);	// disable input - ��ֹ����
			connectBtn->setText(codec->toUnicode(TEXT_DISCONNECT));	// set button "disconnect" - ���ð�ť��ʾ���Ͽ���
			connect_state = 1;
		}
	}
	else	// connected, disconnect it
	{
		disconnect_tcp_server();
	}

}

/* disconnect server  - �Ͽ������� */
void MainWindow::disconnect_tcp_server(void)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	cout << "disconnect server" << endl;

	ipPortEdit->setEnabled(true);
	connectBtn->setText(codec->toUnicode(TEXT_CONNECT));	// set button "connect" - ���ð�ť�����ӡ�
	
	socket_client_stop();	// stop socket client - ֹͣsocket�ͻ���

	connect_state = 0;
}

/* set disconnect server - �Ͽ������� */
void mainwin_set_disconnect(void)
{
	mainwindow->disconnect_tcp_server();
}

/* main window initial - �������ʼ�� */
int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	mainwindow->show();
	
	return 0;
}

