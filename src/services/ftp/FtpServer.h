#ifndef FTPSERVER_H
#define FTPSERVER_H

#include <QSslCertificate>
#include <QSslKey>

#include "../../BaseTcpServer.h"
#include "FtpDataTransfer.h"

class FtpServer : public BaseTcpServer
{
	Q_OBJECT
public:
	enum SslMode {
		None,
		Explicit,
		Implicit,
		Auto
	};

	explicit FtpServer(QSettings &settings, QObject *parent = 0);
	
signals:
	
public slots:

protected:
	void incomingConnection(int socketDescriptor);

private:
	SslMode s_ssl;
	bool s_forceSsl;
	SslMode s_proxySslMode;
	FtpDataTransfer::TransferMode s_proxyMode;
	bool s_proxyActiveMode;
	quint32 s_readBufferSize;
	QSslCertificate r_sslCertificate;
	QSslKey r_sslKey;
	
};

#endif // FTPSERVER_H
