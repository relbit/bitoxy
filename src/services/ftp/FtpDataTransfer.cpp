#include "FtpDataTransfer.h"
#include <QSslKey>

FtpDataTransfer::FtpDataTransfer(TransferMode clientMode, TransferMode serverMode, QObject *parent) :
	QObject(parent),
	clientMode(clientMode),
	serverMode(serverMode),
	clientSocket(0),
	serverSocket(0),
	clientListen(QHostAddress::Any),
	serverListen(QHostAddress::Any),
	connected(0),
	disconnected(0),
	clientSsl(false),
	serverSsl(false),
	clientBufferSize(32768),
	serverBufferSize(32768)
{
	connect(&clientServer, SIGNAL(clientConnected(QSslSocket*)), this, SLOT(clientConnectedPassive(QSslSocket*)));
	connect(&serverServer, SIGNAL(clientConnected(QSslSocket*)), this, SLOT(serverConnectedActive(QSslSocket*)));
}

FtpDataTransfer::~FtpDataTransfer()
{
	clientSocket = 0;
	serverSocket = 0;
}

void FtpDataTransfer::setClient(QHostAddress host, quint16 port)
{
	clientAddress = host;
	clientPort = port;
}

void FtpDataTransfer::setClient(QString host, quint16 port)
{
	setClient(QHostAddress(host), port);
}

void FtpDataTransfer::setClientListenAddress(QHostAddress address)
{
	clientListen = address;
}

void FtpDataTransfer::setServer(QHostAddress host, quint16 port)
{
	serverAddress = host;
	serverPort = port;
}

void FtpDataTransfer::setServer(QString host, quint16 port)
{
	setServer(QHostAddress(host), port);
}

void FtpDataTransfer::setServerListenAddress(QHostAddress address)
{
	serverListen = address;
}

void FtpDataTransfer::setUseSsl(bool clientSsl, bool serverSsl, QSslCertificate cert, QSslKey key)
{
	this->clientSsl = clientSsl;
	this->serverSsl = serverSsl;

	if(clientSsl)
	{
		certificate = cert;
		privateKey = key;
	}
}

void FtpDataTransfer::setClientReadBufferSize(quint32 size)
{
	clientBufferSize = size;
}

void FtpDataTransfer::setServerReadBufferSize(quint32 size)
{
	serverBufferSize = size;
}

QHostAddress FtpDataTransfer::clientServerAddress()
{
	return clientServer.serverAddress();
}

quint16 FtpDataTransfer::clientServerPort()
{
	return clientServer.serverPort();
}

QHostAddress FtpDataTransfer::serverServerAddress()
{
	return serverServer.serverAddress();
}

quint16 FtpDataTransfer::serverServerPort()
{
	return serverServer.serverPort();
}

void FtpDataTransfer::start()
{
	if(clientMode == Active)
	{
		clientSocket = new QSslSocket(this);
		clientSocket->setReadBufferSize(clientBufferSize);

//		connect(clientSocket, SIGNAL(readyRead()), this, SLOT(forwardFromClientToServer()));
//		connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromServerToClient()));
//		connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

//		if(useSsl)
//		{
//			connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnected()));

//			clientSocket->setLocalCertificate(certificate);
//			clientSocket->setPrivateKey(privateKey);
//			//clientSocket->connectToHostEncrypted(clientAddress.toString(), clientPort);

//			// Do not forget that in FTP the server is ALWAYS TLS server and client is ALWAYS TLS client
//			// Even when client is listening
//			clientSocket->connectToHost(clientAddress, clientPort);
//		} else {
			connect(clientSocket, SIGNAL(connected()), this, SLOT(clientConnectedActive()));

			clientSocket->connectToHost(clientAddress, clientPort);
//		}
	} else {
		clientServer.listen(clientListen);
	}

	if(serverMode == Passive)
	{
		serverSocket = new QSslSocket(this);
		serverSocket->setReadBufferSize(serverBufferSize);

//		connect(serverSocket, SIGNAL(readyRead()), this, SLOT(forwardFromServerToClient()));
//		connect(serverSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromClientToServer()));
//		connect(serverSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnected()));

		qDebug() << "Connecting to" << serverAddress << serverPort;

		if(serverSsl)
		{
			connect(serverSocket, SIGNAL(encrypted()), this, SLOT(serverConnectedPassive()));

//			serverSocket->setLocalCertificate(certificate);
//			serverSocket->setPrivateKey(privateKey);
			serverSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
			serverSocket->setProtocol(QSsl::TlsV1);

			// Here we act like client
			serverSocket->connectToHostEncrypted(serverAddress.toString(), serverPort);
		} else {
			connect(serverSocket, SIGNAL(connected()), this, SLOT(serverConnectedPassive()));

			serverSocket->connectToHost(serverAddress, serverPort);
		}
	} else {
		serverServer.listen(serverListen);
	}
}

void FtpDataTransfer::clientConnectedActive()
{
	if(!clientSsl || clientSocket->isEncrypted())
	{
		qDebug() << "Client connected active, ok";

		connect(clientSocket, SIGNAL(readyRead()), this, SLOT(forwardFromClientToServer()));
		connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromServerToClient()));
		connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	} else {
		qDebug() << "Client connected active, engage ssl";

		connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnectedActive()));

		clientSocket->setLocalCertificate(certificate);
		clientSocket->setPrivateKey(privateKey);
		clientSocket->setProtocol(QSsl::TlsV1);
		clientSocket->startServerEncryption();
		clientSocket->waitForEncrypted(1000); // I just don't know... those fucking signals are not working
	}
}

void FtpDataTransfer::serverConnectedPassive()
{
	if(!serverSsl || serverSocket->isEncrypted())
	{
		qDebug() << "Server connected, ok";

		connect(serverSocket, SIGNAL(readyRead()), this, SLOT(forwardFromServerToClient()));
		connect(serverSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromClientToServer()));
		connect(serverSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnected()));
	} else {
		qDebug() << "Server connected, engage ssl";

		connect(serverSocket, SIGNAL(encrypted()), this, SLOT(serverConnectedPassive()));

		serverSocket->startClientEncryption();
	}
}

void FtpDataTransfer::clientConnectedPassive(QSslSocket *client)
{
//	if(client)
//		clientSocket = client;

//	if(!client || !useSsl)
//	{
//		connect(clientSocket, SIGNAL(readyRead()), this, SLOT(forwardFromClientToServer()));
//		connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromServerToClient()));
//		connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
//	}

//	qDebug() << "Client connected to passive server" << clientSocket->peerAddress();

//	if(useSsl && !clientSocket->isEncrypted())
//	{
//		connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnected()));

//		clientSocket->setLocalCertificate(certificate);
//		clientSocket->setPrivateKey(privateKey);
//		clientSocket->startServerEncryption();
//	}

	clientSocket = client;

	clientSocket->setReadBufferSize(clientBufferSize);

	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(forwardFromClientToServer()));
	connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromServerToClient()));
	connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnectedActive()));

	if(!clientSsl || clientSocket->isEncrypted())
	{
		qDebug() << "Client connected passive, ok";


	} else {
		qDebug() << "Client connected passive, engage ssl";



		clientSocket->setLocalCertificate(certificate);
		clientSocket->setPrivateKey(privateKey);
		clientSocket->setProtocol(QSsl::AnyProtocol);
		clientSocket->startServerEncryption();

		clientSocket->waitForEncrypted(1000);// FIXME AAAA
		qDebug()<< "errs" << clientSocket->sslErrors() << clientSocket->protocol();
	}
}

void FtpDataTransfer::serverConnectedActive(QSslSocket *server)
{
//	if(server)
//		serverSocket = server;

//	if(!server || !useSsl)
//	{
//		connect(serverSocket, SIGNAL(readyRead()), this, SLOT(forwardFromServerToClient()));
//		connect(serverSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromClientToServer()));
//		connect(serverSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnected()));
//	}

//	qDebug() << "Server connected to passive server" << serverSocket->peerAddress() << server;

//	if(useSsl && internalSsl && server)
//	{
//		connect(clientSocket, SIGNAL(encrypted()), this, SLOT(serverConnected()));

//		serverSocket->setLocalCertificate(certificate);
//		serverSocket->setPrivateKey(privateKey);
//		serverSocket->startServerEncryption();
//	}

	serverSocket = server;

	serverSocket->setReadBufferSize(serverBufferSize);

	if(!serverSsl || serverSocket->isEncrypted())
	{
		qDebug() << "Server active connected, ok";

		connect(serverSocket, SIGNAL(readyRead()), this, SLOT(forwardFromServerToClient()));
		connect(serverSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromClientToServer()));
		connect(serverSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnected()));
	} else {
		qDebug() << "Server active connected, engage ssl";

		connect(serverSocket, SIGNAL(encrypted()), this, SLOT(serverConnectedPassive()));

//		serverSocket->setLocalCertificate(certificate);
//		serverSocket->setPrivateKey(privateKey);
		serverSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
		serverSocket->setProtocol(QSsl::TlsV1);
		serverSocket->startClientEncryption();

		//qDebug() << serverSocket->waitForEncrypted(1000); // FIXME aaaaa
		qDebug() << serverSocket->sslErrors();

		//qDebug() << serverSocket->waitForEncrypted();
	}
}

void FtpDataTransfer::forwardFromServerToClient()
{
	if(!clientSocket || !serverSocket)
	{
		qDebug() << "Clietn or server not connected!" << clientSocket << serverSocket;
		return;
	}

	if(serverSocket->bytesAvailable() && !clientSocket->bytesToWrite())
	{
		//qDebug() << "DTP: Forwarding from server to client" << clientSocket->write(serverSocket->read(8192)) << "bytes";
		clientSocket->write(serverSocket->read(serverBufferSize));
	}
}

void FtpDataTransfer::forwardFromClientToServer()
{
	if(!clientSocket || !serverSocket)
	{
		qDebug() << "Clietn or server not connected!" << clientSocket << serverSocket;
		return;
	}

	if(clientSocket->bytesAvailable() && !serverSocket->bytesToWrite())
	{
		//qDebug() << "DTP: Forwarding from client to server" << serverSocket->write(clientSocket->read(8192)) << "bytes";
		serverSocket->write(clientSocket->read(clientBufferSize));
	}
}

void FtpDataTransfer::clientDisconnected()
{
	qDebug() << "Client disconnected";

	if(serverSocket)
		serverSocket->disconnectFromHost();

	if(++disconnected == 2)
		emit transferFinished();
}

void FtpDataTransfer::serverDisconnected()
{
	qDebug() << "Server disconnected";

	if(clientSocket)
		clientSocket->disconnectFromHost();

	if(++disconnected == 2)
		emit transferFinished();
}
