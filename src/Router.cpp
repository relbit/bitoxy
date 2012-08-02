#include <QDebug>

#include "Router.h"

QList<Router*> Router::m_routers;

Router::Router(QSettings &settings)
{
}

int Router::registerRouter(Router *router)
{
	m_routers << router;
	return m_routers.count() - 1;
}

QPair<QString, quint16> Router::findRouteForUsername(int router, QString username)
{
	qDebug() << "rtrs" << m_routers.count();

	if(router < 0 || router >= m_routers.count())
		return QPair<QString, quint16>(QString(), 0);

	return m_routers.at(router)->findRouteForUsername(username);
}

QList<Router*> Router::routers()
{
	return m_routers;
}
