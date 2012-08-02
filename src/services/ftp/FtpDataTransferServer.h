#ifndef FTPDATATRANSFERSERVER_H
#define FTPDATATRANSFERSERVER_H

#include <QTcpServer>
#include <QSslSocket>

class FtpDataTransferServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit FtpDataTransferServer(QObject *parent = 0);
	void setUseSsl(bool ssl);
	
signals:
	void clientConnected(QSslSocket *client);
	
public slots:

protected:
	void incomingConnection(int socketDescriptor);

private:
	bool useSsl;
	
};

#endif // FTPDATATRANSFERSERVER_H
