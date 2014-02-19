#ifndef FTPDATATRANSFER_H
#define FTPDATATRANSFER_H

#include <QObject>
#include <QSslSocket>
#include <QSslKey>

#include "FtpDataTransferServer.h"

class FtpDataTransfer : public QObject
{
	Q_OBJECT
public:
	enum TransferMode {
		Active = 0,
		Passive,
		Auto
	};

	explicit FtpDataTransfer(TransferMode clientMode, TransferMode serverMode, QObject *parent = 0);
	~FtpDataTransfer();
	void setClient(QHostAddress host, quint16 port);
	void setClient(QString host, quint16 port);
	void setClientListenAddress(QHostAddress address);
	void setServer(QHostAddress host, quint16 port);
	void setServer(QString host, quint16 port);
	void setServerListenAddress(QHostAddress address);
	void setUseSsl(bool clientSsl, bool serverSsl = false, QSslCertificate cert = QSslCertificate(), QSslKey key = QSslKey());
	QHostAddress clientServerAddress();
	quint16 clientServerPort();
	QHostAddress serverServerAddress();
	quint16 serverServerPort();
	void setClientReadBufferSize(quint32 size);
	void setServerReadBufferSize(quint32 size);
	quint64 bytesTransfered() const;
	
signals:
	void transferFinished();
	
public slots:
	void start();

private:
	TransferMode clientMode;
	TransferMode serverMode;
	QSslSocket *clientSocket;
	QSslSocket *serverSocket;
	FtpDataTransferServer clientServer;
	FtpDataTransferServer serverServer;
	QHostAddress clientAddress;
	quint16 clientPort;
	QHostAddress serverAddress;
	quint16 serverPort;
	QHostAddress clientListen;
	QHostAddress serverListen;
	bool clientSsl;
	bool serverSsl;
	QSslCertificate certificate;
	QSslKey privateKey;
	qint8 connected;
	qint8 disconnected;
	quint32 clientBufferSize;
	quint32 serverBufferSize;
	QByteArray buffer;
	bool clientReady;
	bool serverReady;
	bool clientDone;
	bool serverDone;
	quint64 m_bytesTransfered;

private slots:
	// Naming from the Bitoxy point of view
	void clientConnectedActive();
	void serverConnectedPassive();
	void clientConnectedPassive(QSslSocket *client);
	void serverConnectedActive(QSslSocket *server);
	void forwardFromServerToClient();
	void forwardFromClientToServer();
	void clientDisconnected();
	void serverDisconnected();
	void sslErrors(const QList<QSslError> &errors);
	void peerVerifyError(const QSslError &error);
	void modeChange(QSslSocket::SslMode mode);
};

#endif // FTPDATATRANSFER_H
