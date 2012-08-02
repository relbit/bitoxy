#include <QThread>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "SqlRouter.h"

SqlRouter::SqlRouter(QSettings &settings) :
	Router(settings)
{
	s_driver = settings.value("Driver").toString();
	s_host = settings.value("Host", "localhost").toString();
	s_port = settings.value("Port", 3306).toInt();
	s_database = settings.value("Database").toString();
	s_user = settings.value("User").toString();
	s_password = settings.value("Password").toString();
	s_query = settings.value("Query").toString();
	s_cache = settings.value("Cache", true).toBool();
	s_ttl = settings.value("TTL", 60).toInt();
}

SqlRouter::~SqlRouter()
{
	if(s_cache)
		qDeleteAll(cache);
}

QPair<QString, quint16> SqlRouter::findRouteForUsername(QString username)
{
	QMutexLocker locker(&mutex);

	bool inCache = false;

	if(s_cache && cache.contains(username))
	{
		CacheEntry *ce = cache.value(username);

		if(ce->time > QDateTime::currentDateTime())
		{
			qDebug() << "SqlRouter (cache hit):" << username << "->" << ce->server.first << ce->server.second;
			return ce->server;
		}

		inCache = true;
	}

	QString connection = QString::number((qint64)QThread::currentThread());
	QString host;
	quint16 port = 0;
	QSqlDatabase db;

	if(QSqlDatabase::contains(connection))
		db = QSqlDatabase::database(connection);
	else {
		db = QSqlDatabase::addDatabase(s_driver.toUpper(), connection);
		db.setHostName(s_host);
		db.setPort(s_port);
		db.setDatabaseName(s_database);
		db.setUserName(s_user);
		db.setPassword(s_password);
	}

	if(!db.open())
	{
		qWarning() << "Cannot connect to database:" << db.lastError();
		return QPair<QString, quint16>(QString(), 0);
	}

	{
		QSqlQuery query(db);
		query.prepare(s_query);
		query.bindValue(":username", username);
		query.exec();

		query.first();

		if(query.isValid())
		{
			host = query.value(0).toString();
			port = query.value(1).toInt();
		}
	}

	db.close();

	qDebug() << "SqlRouter:" << username << "->" << host << port;

	if(s_cache)
	{
		CacheEntry *ce;

		if(inCache)
			ce = cache.value(username);
		else ce = new CacheEntry;

		ce->server = QPair<QString, quint16>(host, port);
		ce->time = QDateTime::currentDateTime().addSecs(s_ttl);

		if(!inCache)
			cache[username] = ce;
	}

	return QPair<QString, quint16>(host, port);
}
