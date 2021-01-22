#include "wndmain.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	wndMain w;
	w.show();

	return a.exec();
}
