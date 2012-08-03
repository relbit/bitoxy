#include "FtpDataTransferServer.h"

FtpDataTransferServer::FtpDataTransferServer(QObject *parent) :
        QTcpServer(parent)
{
	setMaxPendingConnections(1);
}

void FtpDataTransferServer::incomingConnection(int socketDescriptor)
{
	QSslSocket *con = new QSslSocket(this);
	con->setSocketDescriptor(socketDescriptor);

	emit clientConnected(con);
}
