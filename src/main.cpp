#include <QtCore/QCoreApplication>
#include <QDebug>

#include "Bitoxy.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	Bitoxy bit;

	if(bit.init())
		return a.exec();
	else {
		qWarning() << "Bitoxy init failed, exiting.";
		return 1;
	}
}
