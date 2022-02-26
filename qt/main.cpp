#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <QApplication>
#include "mainwindow.h"

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif


using namespace std;



int main(int argc, char* argv[])
{
    QApplication qtApp(argc, argv);
	
	cout << "hello main" << endl;

	/* main window init - 主界面显示初始化 */
	mainwindow_init();

	return qtApp.exec();		// start qt application, message loop ...
}
