#ifndef STATICROUTER_H
#define STATICROUTER_H

#include "../Router.h"

class StaticRouter : public Router
{
public:
	StaticRouter(QSettings &settings);
	QPair<QString, quint16> findRouteForUsername(QString username);

private:
	QString s_host;
	quint16 s_port;
	
};

#endif // STATICROUTER_H
