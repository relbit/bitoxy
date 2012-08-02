#include <QThread>

#include "Worker.h"

#include "services/ftp/FtpConnection.h"

Worker::Worker(QObject *parent) :
	QObject(parent)
{
}

//void Worker::run()
//{
//	qDebug() << "Thread running";

//	exec();
//}

void Worker::addConnection(IncomingConnection incomingConnection)
{
//	connectionQueue.enqueue(connection);

//	qDebug() << "Connection queued in thread" << QThread::currentThread();

	BaseConnection *con;

	switch(incomingConnection.serviceType)
	{
#ifdef SERVICE_FTP
	case BaseTcpServer::FtpService:
	{
		con = new FtpConnection(this);

		break;
	}
#endif
#ifdef SERVICE_SSH
	case BaseTcpServer::SshService:
	{

		break;
	}
#endif
	default:
		return;
	}

	con->applySettings(incomingConnection.settings);
	con->setSocketDescriptor(incomingConnection.socketDescriptor);

	connect(con, SIGNAL(disconnected(BaseConnection*)), this, SLOT(connectionDisconnected(BaseConnection*)));

	connections << con;

	qDebug() << "Connection created in thread" << QThread::currentThread();
}

//void Worker::processIncomingConnections()
//{
//	if(connectionQueue.isEmpty())
//	{
//		qDebug() << "Queue empty";
//		return;
//	}

//	IncomingConnection incomingConnection = connectionQueue.dequeue();

//	BaseConnection *con;

//	switch(incomingConnection.serviceType)
//	{
//#ifdef SERVICE_FTP
//	case BaseTcpServer::FtpService:
//	{
//		con = new FtpConnection(this);

//		break;
//	}
//#endif
//#ifdef SERVICE_SSH
//	case BaseTcpServer::SshService:
//	{

//		break;
//	}
//#endif
//	default:
//		return;
//	}

//	con->setSocketDescriptor(incomingConnection.socketDescriptor);

//	connect(con, SIGNAL(disconnected(BaseConnection*)), this, SLOT(connectionDisconnected(BaseConnection*)));

//	connections << con;

//	qDebug() << "Connection created in thread" << QThread::currentThread();
//}

int Worker::connectionCount()
{
	return connections.count();
}

void Worker::connectionDisconnected(BaseConnection *connection)
{
	connections.removeOne(connection);
	connection->deleteLater();

	qDebug() << "Connection scheduled for deletion";
}
