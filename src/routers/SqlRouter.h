#ifndef SQLROUTER_H
#define SQLROUTER_H

#include <QHash>
#include <QMutex>
#include <QDateTime>

#include "../Router.h"

class SqlRouter : public Router
{
public:
	SqlRouter(QSettings &settings);
	virtual ~SqlRouter();
	QPair<QString, quint16> findRouteForUsername(QString username);

private:
	struct CacheEntry
	{
		QPair<QString, quint16> server;
		QDateTime time;
	};

	QHash<QString, CacheEntry*> cache;
	mutable QMutex mutex;

	QString s_driver;
	QString s_host;
	quint16 s_port;
	QString s_database;
	QString s_user;
	QString s_password;
	QString s_query;
	bool s_cache;
	int s_ttl;
	
};

#endif // SQLROUTER_H
