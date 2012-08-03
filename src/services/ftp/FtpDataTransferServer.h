#ifndef FTPDATATRANSFERSERVER_H
#define FTPDATATRANSFERSERVER_H

#include <QTcpServer>
#include <QSslSocket>

class FtpDataTransferServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit FtpDataTransferServer(QObject *parent = 0);
	
signals:
	void clientConnected(QSslSocket *client);
	
public slots:

protected:
	void incomingConnection(int socketDescriptor);
	
};

#endif // FTPDATATRANSFERSERVER_H
