#include <QDebug>

#include "Router.h"

QList<Router*> Router::m_routers;

Router::Router(QSettings &settings)
{
	Q_UNUSED(settings);
}

Router::~Router()
{

}

int Router::registerRouter(Router *router)
{
	m_routers << router;
	return m_routers.count() - 1;
}

QPair<QString, quint16> Router::findRouteForUsername(int router, QString username)
{
	if(router < 0 || router >= m_routers.count())
		return QPair<QString, quint16>(QString(), 0);

	return m_routers.at(router)->findRouteForUsername(username);
}

QList<Router*> Router::routers()
{
	return m_routers;
}
