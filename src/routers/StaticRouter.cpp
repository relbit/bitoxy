#include <QDebug>

#include "StaticRouter.h"

StaticRouter::StaticRouter(QSettings &settings) :
	Router(settings)
{
	s_host = settings.value("Host").toString();
	s_port = settings.value("Port").toInt();
}

QPair<QString, quint16> StaticRouter::findRouteForUsername(QString username)
{
	qDebug() << "StaticRouter:" << username << "->" << s_host << s_port;

	return QPair<QString, quint16>(s_host, s_port);
}
