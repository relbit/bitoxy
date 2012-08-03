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

QVariant BaseTcpServer::findConfigValue(QSettings &settings, QString var)
{
	QString originGroup = settings.group();

	while(settings.value(var).isNull() && !settings.group().isEmpty())
	{
		settings.endGroup();

		originGroup.insert(0, settings.group() + "/");
	}

	QVariant val = settings.value(var);

	if(originGroup != (settings.group() + "/"))
		settings.beginGroup(originGroup);

	return val;
}
