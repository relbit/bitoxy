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
	serverBufferSize(32768),
	clientReady(false),
	serverReady(false),
	clientDone(false),
	serverDone(false),
	m_bytesTransfered(0)
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

quint64 FtpDataTransfer::bytesTransfered() const
{
	return m_bytesTransfered;
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

//		if(clientSsl)
//		{
//			connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnectedActive()));

//			clientSocket->setLocalCertificate(certificate);
//			clientSocket->setPrivateKey(privateKey);

//			clientSocket->connectToHostEncrypted(clientAddress.toString(), clientPort);

//			// Do not forget that in FTP the server is ALWAYS TLS server and client is ALWAYS TLS client
//			// Even when client is listening
////			clientSocket->connectToHost(clientAddress, clientPort);
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
			serverSocket->setProtocol(QSsl::AnyProtocol);

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

		if(!clientSocket->isEncrypted())
		{
			connect(clientSocket, SIGNAL(readyRead()), this, SLOT(forwardFromClientToServer()));
			connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromServerToClient()));
			connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
		}

		clientReady = true;

		if(serverDone)
		{
			clientSocket->write(buffer);
			buffer.clear();
			clientSocket->disconnectFromHost();
		}
	} else {
		qDebug() << "Client connected active, engage ssl";

		connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnectedActive()));

		clientSocket->setLocalCertificate(certificate);
		clientSocket->setPrivateKey(privateKey);
		clientSocket->setProtocol(QSsl::AnyProtocol);
		clientSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
		clientSocket->startServerEncryption();
	//	clientSocket->waitForEncrypted(1000); // I just don't know... those fucking signals are not working
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

		serverReady = true;

		if(clientDone)
		{
			serverSocket->write(buffer);
			buffer.clear();
			serverSocket->disconnectFromHost();
		}
	} else {
		qDebug() << "Server connected, engage ssl";

		connect(serverSocket, SIGNAL(encrypted()), this, SLOT(serverConnectedPassive()));

		serverSocket->startClientEncryption();
	}
}

void FtpDataTransfer::clientConnectedPassive(QSslSocket *client)
{
	clientSocket = client;

	clientSocket->setReadBufferSize(clientBufferSize);

	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(forwardFromClientToServer()));
	connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromServerToClient()));
	connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	connect(clientSocket, SIGNAL(encrypted()), this, SLOT(clientConnectedActive()));
	connect(clientSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
	connect(clientSocket, SIGNAL(peerVerifyError(QSslError)), this, SLOT(peerVerifyError(QSslError)));
	connect(clientSocket, SIGNAL(modeChanged(QSslSocket::SslMode)), this, SLOT(modeChange(QSslSocket::SslMode)));

	if(!clientSsl || clientSocket->isEncrypted())
	{
		qDebug() << "Client connected passive, ok";
		clientReady = true;

		if(serverDone)
		{
			clientSocket->write(buffer);
			buffer.clear();
			clientSocket->disconnectFromHost();
		}
	} else {
		qDebug() << "Client connected passive, engage ssl";

		clientSocket->setLocalCertificate(certificate);
		clientSocket->setPrivateKey(privateKey);
		clientSocket->setProtocol(QSsl::AnyProtocol);
		clientSocket->setPeerVerifyMode(QSslSocket::VerifyNone);

		QList<QSslError> ignoreErrors;
		ignoreErrors << QSslError(QSslError::SelfSignedCertificate);
		clientSocket->ignoreSslErrors(ignoreErrors);

		clientSocket->startServerEncryption();

//		clientSocket->waitForEncrypted(1000);// FIXME AAAA
	}
}

void FtpDataTransfer::serverConnectedActive(QSslSocket *server)
{
	serverSocket = server;

	serverSocket->setReadBufferSize(serverBufferSize);

	if(!serverSsl || serverSocket->isEncrypted())
	{
		qDebug() << "Server active connected, ok";

		if(!serverSocket->isEncrypted())
		{
			connect(serverSocket, SIGNAL(readyRead()), this, SLOT(forwardFromServerToClient()));
			connect(serverSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(forwardFromClientToServer()));
			connect(serverSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnected()));
		}

		serverReady = true;

		if(clientDone)
		{
			serverSocket->write(buffer);
			buffer.clear();
			serverSocket->disconnectFromHost();
		}
	} else {
		qDebug() << "Server active connected, engage ssl";

		connect(serverSocket, SIGNAL(encrypted()), this, SLOT(serverConnectedPassive()));

//		serverSocket->setLocalCertificate(certificate);
//		serverSocket->setPrivateKey(privateKey);
		serverSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
		serverSocket->setProtocol(QSsl::AnyProtocol);
		serverSocket->startClientEncryption();

		//qDebug() << serverSocket->waitForEncrypted(1000); // FIXME aaaaa
		qDebug() << serverSocket->sslErrors();

		//qDebug() << serverSocket->waitForEncrypted();
	}
}

void FtpDataTransfer::forwardFromServerToClient()
{
	if(!clientSocket && serverSocket)
	{
		if(serverSocket->bytesAvailable())
			buffer.append(serverSocket->read(serverBufferSize));

	} else if(clientSocket && !serverSocket) {
		if(!buffer.isEmpty() && clientReady)
		{
			clientSocket->write(buffer);
			buffer.clear();
		}

	} else if(clientReady) {
		if(!buffer.isEmpty())
		{
			clientSocket->write(buffer);
			buffer.clear();
		}

		clientSocket->write(serverSocket->read(serverBufferSize));
	} else {
		buffer.append(serverSocket->read(serverBufferSize));
	}
}

void FtpDataTransfer::forwardFromClientToServer()
{
	if(!serverSocket && clientSocket)
	{
		if(clientSocket->bytesAvailable())
			buffer.append(clientSocket->read(clientBufferSize));

	} else if(serverSocket && !clientSocket) {
		if(!buffer.isEmpty() && serverReady)
		{
			serverSocket->write(buffer);
			buffer.clear();
		}

	} else if(serverReady) {
		if(!buffer.isEmpty())
		{
			serverSocket->write(buffer);
			buffer.clear();
		}

		serverSocket->write(clientSocket->read(clientBufferSize));
	} else {
		buffer.append(clientSocket->read(clientBufferSize));
	}
}

void FtpDataTransfer::clientDisconnected()
{
	qDebug() << "Client disconnected";

	if(serverSocket && serverReady)
		serverSocket->disconnectFromHost();

	clientReady = false;
	clientDone = true;

	if(++disconnected == 2)
		emit transferFinished();
}

void FtpDataTransfer::serverDisconnected()
{
	qDebug() << "Server disconnected";

	if(clientSocket && clientReady)
		clientSocket->disconnectFromHost();

	serverReady = false;
	serverDone = true;

	if(++disconnected == 2)
		emit transferFinished();
}

void FtpDataTransfer::sslErrors(const QList<QSslError> &errors)
{
	qDebug() << "SSL errors occured:" << errors;
}

void FtpDataTransfer::peerVerifyError(const QSslError &error)
{
	QSslCertificate cert = error.certificate();

	qDebug() << "Peer verify error occured:" << (int)error.error() << error
		 << cert.issuerInfo(QSslCertificate::CommonName)
		    << cert.subjectInfo(QSslCertificate::CommonName);
	clientSocket->ignoreSslErrors();
	qDebug() << "Ignoring error";
}

void  FtpDataTransfer::modeChange(QSslSocket::SslMode mode)
{
	qDebug() << "Client ssl mode changed to" << mode;
}
