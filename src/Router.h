#ifndef ROUTER_H
#define ROUTER_H

#include <QPair>
#include <QSettings>

class Router
{
public:
	Router(QSettings &settings);
	virtual ~Router();
	virtual QPair<QString, quint16> findRouteForUsername(QString username) = 0;
	static int registerRouter(Router *router);
	static QPair<QString, quint16> findRouteForUsername(int router, QString username);
	static QList<Router*> routers();

private:
	static QList<Router*> m_routers;
	
};

#endif // ROUTER_H
