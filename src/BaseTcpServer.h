#ifndef BASETCPSERVER_H
#define BASETCPSERVER_H

#define SERVICE_FTP
//#define SERVICE_SSH

#include <QTcpServer>
#include <QSettings>
#include <QHash>

#include "Router.h"

struct IncomingConnection;

class BaseTcpServer : public QTcpServer
{
	Q_OBJECT
public:
	enum ServiceType {
#ifdef SERVICE_FTP
		FtpService,
#endif
#ifdef SERVICE_SSH
		SshService,
#endif
		ServiceUndefined
	};

	explicit BaseTcpServer(QSettings &settings, QObject *parent = 0);
	void setRouter(int r);
	
signals:
	void newConnection(IncomingConnection incomingConnection);
	
public slots:

protected:
	int router;

private:
	
};

struct IncomingConnection
{
	IncomingConnection(){}
	IncomingConnection(const IncomingConnection &other)
	{
		serviceType = other.serviceType;
		socketDescriptor = other.socketDescriptor;
		settings = other.settings;
	}
	~IncomingConnection(){}

	BaseTcpServer::ServiceType serviceType;
	int socketDescriptor;
	QHash<QString, QVariant> settings;
};

#endif // BASETCPSERVER_H
