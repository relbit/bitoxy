#ifndef FTPCONNECTION_H
#define FTPCONNECTION_H

#include <QTcpServer>
#include <QSslSocket>
#include <QList>
#include <QQueue>
#include <QPair>

#include "src/BaseConnection.h"
#include "FtpDataTransfer.h"
#include "FtpServer.h"

class FtpConnection : public BaseConnection
{
	Q_OBJECT
public:
	explicit FtpConnection(QObject *parent = 0);
	void applySettings(QHash<QString, QVariant> &settings);
	
signals:
	
public slots:

protected:
	void replyClient(QString msg);
	void replyClient(int code, QString msg);
	void replyServer(QString msg);
	void replyServer(QString cmd, QString msg);
	//void commandToTargetServer(QString cmd);

private:
	enum TargetServerConnectionState {
		Unconnected = 0,
		HangingUp,
		Connected,
		Failed
	};

	enum FtpCommandType {
		Greetings = 0,
		Auth,
		Pbsz,
		Prot,
		User,
		Pass,
		Port,
		Pasv,
		Eprt,
		Epsv,
		Raw
	};

	enum FtpCommandFlags {
		NoFlags = 1,
		NoForwardResponse = 2,
		ActiveToPassiveTranslation = 4
	};

	struct FtpCommand {
		FtpCommandType type;
		QString str;
		FtpCommandFlags flags;
	};

	void enqueueServerCommand(FtpCommandType cmd, QString arg = QString(), FtpCommandFlags flags = NoFlags);
	void enqueueServerCommand(QString cmd, QString arg);
	void enqueueServerCommand(QString cmd);
	void dispatchServerCommand();
	void dispatchServerCommand(FtpCommandType cmd, QString arg = QString(), FtpCommandFlags flags = NoFlags);
	void dispatchServerCommand(QString cmd, QString arg);
	void dispatchServerCommand(QString cmd);
	QPair<QHostAddress, quint16> decodeHostAndPort(QString msg);
	QPair<QHostAddress, quint16> decodeExtendedHostAndPort(QString msg);
	QString encodeHostAndPort(QHostAddress addr, quint16 port);
	QString encodeExtendedHostAndPort(QHostAddress addr, quint16 port);
	void engageActiveDataConnection(QHostAddress host, quint16 port);
	void engagePassiveDataConnection(QHostAddress host, quint16 port);

	FtpDataTransfer *dataTransfer;
	TargetServerConnectionState targetServerConnectionState;
	QString userName;
	QString password;

	FtpServer::SslMode s_ssl;
	bool s_forceSsl;
	FtpServer::SslMode s_proxySslMode;
	FtpDataTransfer::TransferMode s_proxyMode;
	bool s_proxyActiveMode;
	quint32 s_readBufferSize;

	int r_router;

	bool useSsl;
	bool useSslOnData;
	QQueue<FtpCommand*> serverCommands;
	bool commandSent;
	bool waitForCommand;
	bool userCmdSent;
	bool loggedOnServer;
	QPair<QHostAddress, quint16> lastPortAddr;

private slots:
	void sendGreetings();
	void processCommand();
	void forwardTargetServerReply();
//	void handleDataTransfer(QSslSocket *client);
//	void forwardDataFromTargetServerToClient();
//	void forwardDataFromClientToTargetServer();
//	void disconnectDataConnection();
//	void serverDataConnectionDisconnected();
//	void clientDataConnectionDisconnected();
//	void dataConnectionBytesWritten(qint64 bytes);
	void controlConnectionStateChange(QAbstractSocket::SocketState socketState);
	void controlConnectionEstablished();
	void targetServerConnectionError(QAbstractSocket::SocketError error);
	void targetServerConnectionEncrypted();
	void dataTransferFinished();
};

#endif // FTPCONNECTION_H
