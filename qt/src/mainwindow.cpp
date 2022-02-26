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


/* QT: main window layout - 主界面布局 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	QImage image;

	/* set window title - 设置窗口标题 */
	setWindowTitle(codec->toUnicode(DEFAULT_WINDOW_TITLE));

	/* set window size - 设置窗口大小 */
	resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

	/* set central widget - 设置主控件 */
	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	/* set clock widget - 设置时钟控件 */
	clockLabel = new QLabel(mainWindow);
	clockLabel->setWordWrap(true);	// adapt to text, can show multi row
	clockLabel->setGeometry(10, 10, 200, 20);	// set geometry - 设置几何大小坐标
	clockLabel->show();	// display widget - 显示控制

	/* create timer to show main window - 创建定时器来显示主界面 */
	timer = new QTimer(this);
	/* if timer timeout it will call slot function - 如果定时器定时时间到了就会调用槽函数showMainwindow() */
	connect(timer, SIGNAL(timeout()), this, SLOT(showMainwindow()));
	/* set timeout time and start timer - 设置超时时间并启动定时器，TIMER_INTERV_MS=1ms */
	timer->start(TIMER_INTERV_MS);

	/* input (ip:port) widget - 输入控件（IP地址：端口） */
	ipPortEdit = new QLineEdit(mainWindow);
    ipPortEdit->setPlaceholderText(codec->toUnicode(TEXT_IP_PORT));
    ipPortEdit->setAlignment(Qt::AlignCenter);
	ipPortEdit->setGeometry(10, 100, 150, 30);

	connectBtn = new QPushButton(mainWindow);	// connect/disconnect button - 连接/断开按钮
	connectBtn->setText(codec->toUnicode(TEXT_CONNECT));
	connectBtn->setGeometry(55, 140, 45, 25);
	/* button slot function - 按键的槽函数，信号与槽：当触发了按键时，会调用到槽函数 */
	connect(connectBtn, SIGNAL(clicked()), this, SLOT(connect_tcp_server()));
	connectBtn->setFocus();

	/* user information image - 用户信息图片 */
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

/* timer timeout slot function, to show main window - 定时器定时时间到的槽函数，显示主界面 */
void MainWindow::showMainwindow(void)
{

	timer->stop();

	/* update clock - 更新时间显示 */
	QDateTime time = QDateTime::currentDateTime();
	QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
	clockLabel->setText(str);


	/* start timer again - 再次启动定时器 */
	timer->start(TIMER_INTERV_MS);

}

/* connect server  - 连接服务器 */
void MainWindow::connect_tcp_server(void)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	QString ipPort_qstr;
	QByteArray ba;
	char ip_str[32];
	int ret;
	
	if(connect_state == 0)	// disconnected, connect it
	{
		/* get input content - 获取输入的内容 */
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

		/* start client socket to connect server - 启动客户端socket连接服务器 */
		ret = socket_client_start(ip_str);
		if(ret == 0)
		{
			ipPortEdit->setEnabled(false);	// disable input - 禁止输入
			connectBtn->setText(codec->toUnicode(TEXT_DISCONNECT));	// set button "disconnect" - 设置按钮显示“断开”
			connect_state = 1;
		}
	}
	else	// connected, disconnect it
	{
		disconnect_tcp_server();
	}

}

/* disconnect server  - 断开服务器 */
void MainWindow::disconnect_tcp_server(void)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	cout << "disconnect server" << endl;

	ipPortEdit->setEnabled(true);
	connectBtn->setText(codec->toUnicode(TEXT_CONNECT));	// set button "connect" - 设置按钮“连接”
	
	socket_client_stop();	// stop socket client - 停止socket客户端

	connect_state = 0;
}

/* set disconnect server - 断开服务器 */
void mainwin_set_disconnect(void)
{
	mainwindow->disconnect_tcp_server();
}

/* main window initial - 主界面初始化 */
int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	mainwindow->show();
	
	return 0;
}

