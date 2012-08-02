#include "FtpDataTransferServer.h"

FtpDataTransferServer::FtpDataTransferServer(QObject *parent) :
        QTcpServer(parent)
{
	setMaxPendingConnections(1);
}

void FtpDataTransferServer::setUseSsl(bool ssl)
{
	useSsl = ssl;
}

void FtpDataTransferServer::incomingConnection(int socketDescriptor)
{
	QSslSocket *con = new QSslSocket(this);
	con->setSocketDescriptor(socketDescriptor);
//	con->setReadBufferSize(16384);

//	if(useSsl)
//	{
//		con->setLocalCertificate("/home/aither/dev/cpp/bitoxy/bitoxy.crt");
//		con->setPrivateKey("/home/aither/dev/cpp/bitoxy/bitoxy.key");
//		con->startServerEncryption();

//		// FIXME: solve it by signal :)
//		con->waitForEncrypted();
//	}
//	close();

	emit clientConnected(con);
}
