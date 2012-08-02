#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QList>
#include <QQueue>

#include "BaseTcpServer.h"

class BaseTcpServer;
class BaseConnection;

class Worker : public QObject
{
	Q_OBJECT
public:
	explicit Worker(QObject *parent = 0);
	int connectionCount();
	
signals:
	
public slots:
//	void processIncomingConnections();
	void addConnection(IncomingConnection incomingConnection);

private:
	QQueue<IncomingConnection> connectionQueue;
	QList<BaseConnection*> connections;

private slots:
	void connectionDisconnected(BaseConnection *connection);
	
};

#endif // WORKER_H
