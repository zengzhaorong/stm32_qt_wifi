#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QTextCodec>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDebug>

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif


#define TIMER_INTERV_MS			1

#define TEXT_IP_PORT			"请输入 IP:Port"
#define TEXT_CONNECT			"连接"
#define TEXT_DISCONNECT			"断开"

#define USER_INFO_IMG		"resource/user_info.png"

using namespace std;


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void showMainwindow(void);
	void connect_tcp_server(void);
	
private:
	QWidget			*mainWindow;		// main window
	QLabel			*clockLabel;		// display clock
    QLineEdit       *ipPortEdit;        // input ip and port, format <ip:port>
	QPushButton		*connectBtn;		// connect button
	QLabel 			*userInfo;			// user information, show as image
	QTimer 			*timer;				// display timer

	int connect_state;	// connect state, 0-disconnect, 1-connect

private slots:

public:
	void disconnect_tcp_server(void);
	
};


void mainwin_set_disconnect(void);

int mainwindow_init(void);


#endif	// _MAINWINDOW_H_