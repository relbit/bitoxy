#include "BaseTcpServer.h"

BaseTcpServer::BaseTcpServer(QSettings &settings, QObject *parent) :
	QTcpServer(parent)
{

	qRegisterMetaType<IncomingConnection>("IncomingConnection");
}

void BaseTcpServer::setRouter(int r)
{
	router = r;
}
