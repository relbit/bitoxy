#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QRegExp>
#include <QStringList>
#include <QDebug>
#include <QSslCipher>

#include "FtpConnection.h"
#include "../../Router.h"

FtpConnection::FtpConnection(QObject *parent) :
	BaseConnection(parent),
	dataTransfer(0),
	targetServerConnectionState(FtpConnection::Unconnected),
	useSsl(false),
	useSslOnData(false),
	commandSent(false),
	waitForCommand(false),
	userCmdSent(false),
	loggedOnServer(false)
{
	connect(this, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(controlConnectionStateChange(QAbstractSocket::SocketState)));
//	connect(this, SIGNAL(connected()), this, SLOT(sendGreetings()));
	connect(this, SIGNAL(readyRead()), this, SLOT(processCommand()));
	//connect(this, SIGNAL(disconnected()), this, SLOT(disconnectDataConnection())); FIXME?
//	connect(&dataTransfer, SIGNAL(clientConnected(QSslSocket*)), this, SLOT(handleDataTransfer(QSslSocket*)));

	setLocalCertificate("/home/aither/dev/cpp/bitoxy/bitoxy.crt");
	setPrivateKey("/home/aither/dev/cpp/bitoxy/bitoxy.key");
	setProtocol(QSsl::TlsV1);
}

void FtpConnection::applySettings(QHash<QString, QVariant> &settings)
{
	qDebug() << "Applying ftp con settings" << settings;

	s_ssl = (FtpServer::SslMode)settings["Ssl"].toInt();
	s_forceSsl = settings["ForceSsl"].toBool();
	s_proxySslMode = (FtpServer::SslMode)settings["ProxySslMode"].toInt();
	s_proxyMode = (FtpDataTransfer::TransferMode)settings["ProxyMode"].toInt();
	s_proxyActiveMode = settings["ProxyActiveMode"].toBool();
	s_readBufferSize = settings["ReadBufferSize"].toInt();

	r_router = settings["Router"].toInt();

	if(settings.contains("SSLCertificate"))
	{
		sslCertificate = QSslCertificate::fromData(settings["SSLCertificate"].toByteArray()).first();
		sslKey = QSslKey(settings["SSLPrivateKey"].toByteArray(), QSsl::Rsa);
	}
}

void FtpConnection::sendGreetings()
{
	replyClient(220, "Bitoxy at your service.");
}

void FtpConnection::replyClient(QString msg)
{
	qDebug() << "Sending reply to client" << msg;

	write((msg + "\r\n").toAscii());
}

void FtpConnection::replyClient(int code, QString msg)
{
	replyClient(QString("%1 %2").arg(code).arg(msg));
}

void FtpConnection::replyServer(QString msg)
{
	qDebug() << "Sending reply to server" << msg.toAscii();

	targetServer->write((msg + "\r\n").toAscii());
}

void FtpConnection::replyServer(QString cmd, QString msg)
{
	replyServer(QString("%1 %2").arg(cmd).arg(msg));
}

void FtpConnection::processCommand()
{
	while(bytesAvailable())
	{
		QByteArray rawMsg = readLine();
		QString msg(rawMsg);
		msg = msg.trimmed();

		qDebug() << "Received msg" << msg;

		QString cmd = msg.section(' ', 0, 0);

		qDebug() << "Command identified as" << cmd;

		if(!cmd.compare("USER", Qt::CaseInsensitive))
		{
			qDebug() << "User is trying to log in" << msg.section(' ', 1, 1);
			//sendReply(331, "Please specify the password.");
			if(targetServer)
			{
				//targetServer->write(rawMsg);
				replyClient(503, "You are already logged in.");
			} else {
				if(s_forceSsl && !useSsl)
				{
					replyClient(550, "SSL/TLS required.");
					return;
				}

				userName = msg.section(' ', 1, 1);

				if(userName.isEmpty())
				{
					replyClient(500, "USER: command requires a parameter.");
					return;
				}

				QPair<QString, quint16> server = Router::findRouteForUsername(r_router, userName);
				// FIXME: do some checking if not found and so on

				if(server.second)
				{
					targetServer = new QSslSocket(this);
					connect(targetServer, SIGNAL(connected()), this, SLOT(controlConnectionEstablished()));
					connect(targetServer, SIGNAL(readyRead()), this, SLOT(forwardTargetServerReply()));
					connect(targetServer, SIGNAL(encrypted()), this, SLOT(targetServerConnectionEncrypted()));
					connect(targetServer, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(targetServerConnectionError(QAbstractSocket::SocketError)));

					targetServer->connectToHost(server.first, server.second);

					enqueueServerCommand(Greetings);
					waitForCommand = true;
					commandSent = true;
				} else {
					replyClient(331, "Please specify the password.");
				}
			}
		} else if(!cmd.compare("PASS", Qt::CaseInsensitive)) {
			if(!targetServer && !userName.isEmpty())
			{
				replyClient(530, "Login incorrect.");
			} else if(!targetServer)
				replyClient(503, "Login with USER first.");
			else if(targetServer && targetServerConnectionState == Failed) {
				replyClient(421, "Service not available.");
				disconnectFromHost();
			} else {
				password = msg.section(' ', 1, 1);

				if(userCmdSent)
					dispatchServerCommand(Pass, password);
			}
		} else if(cmd == "PORT") {
			if(!targetServer)
				replyClient(530, "Please login with USER and PASS.");
			else if(s_proxyActiveMode)
			{
				qDebug() << "Proxying active mode";
				if(dataTransfer)
				{
					dataTransfer->deleteLater();
				}

				QPair<QHostAddress, quint16> addr = decodeHostAndPort(msg);

				if(addr.second > 0)
				{
					if(s_proxyMode == FtpDataTransfer::Passive)
					{
						lastPortAddr = addr;
						dispatchServerCommand(Pasv, "", ActiveToPassiveTranslation);

						return;
					}

					engageActiveDataConnection(addr.first, addr.second);

					dispatchServerCommand(Port, encodeHostAndPort(dataTransfer->serverServerAddress(), dataTransfer->serverServerPort()));
				}
			} else {
				qDebug() << "Just forwarding to server, no need to proxy";
				dispatchServerCommand(msg);
				//targetServer->write(rawMsg);
			}
		} else if(!cmd.compare("PASV", Qt::CaseInsensitive)) {
			if(!targetServer)
				replyClient(530, "Please login with USER and PASS.");
			else {
				if(s_proxyMode == FtpDataTransfer::Active)
				{
					if(dataTransfer)
					{
						dataTransfer->deleteLater();
					}

					qDebug() << "Proxying in ACTIVE mode";

					engagePassiveToActiveDataConnectionTranslation();

					dispatchServerCommand(Port, encodeHostAndPort(dataTransfer->serverServerAddress(), dataTransfer->serverServerPort()), NoForwardResponse);
					replyClient(227, QString("Entering Passive Mode (%1)").arg(encodeHostAndPort(dataTransfer->clientServerAddress(), dataTransfer->clientServerPort())));
				} else
					dispatchServerCommand(Pasv);
			}
		} else if(!cmd.compare("EPRT", Qt::CaseInsensitive)) {
			if(!targetServer)
				replyClient(530, "Please login with USER and PASS.");
			else if(s_proxyActiveMode) {

				qDebug() << "Proxying active mode";

				if(dataTransfer)
				{
					dataTransfer->deleteLater();
				}

				QPair<QHostAddress, quint16> addr = decodeExtendedHostAndPort(msg);

				if(addr.second > 0)
				{
					if(s_proxyMode == FtpDataTransfer::Passive)
					{
						lastPortAddr = addr;
						dispatchServerCommand(Epsv, "", ActiveToPassiveTranslation);

						return;
					}

					engageActiveDataConnection(addr.first, addr.second);

					dispatchServerCommand(Eprt, encodeExtendedHostAndPort(dataTransfer->serverServerAddress(), dataTransfer->serverServerPort()));
				}
			} else {
				dispatchServerCommand(msg);
			}
		} else if(!cmd.compare("EPSV", Qt::CaseInsensitive)) {
			if(!targetServer)
				replyClient(530, "Please login with USER and PASS.");
			else if(s_proxyMode == FtpDataTransfer::Active) {
				if(dataTransfer)
				{
					dataTransfer->deleteLater();
				}

				qDebug() << "Proxying in ACTIVE mode";
				engagePassiveToActiveDataConnectionTranslation();

				dispatchServerCommand(Eprt, encodeExtendedHostAndPort(dataTransfer->serverServerAddress(), dataTransfer->serverServerPort()), NoForwardResponse);
				replyClient(227, QString("Entering Extended Passive Mode (%1)").arg(encodeExtendedHostAndPort(dataTransfer->clientServerAddress(), dataTransfer->clientServerPort())));

			} else
				dispatchServerCommand(Epsv);
		} else if(!cmd.compare("AUTH", Qt::CaseInsensitive)) {
			if(s_ssl == FtpServer::Explicit)
			{
				useSsl = true;
				replyClient(234, "Proceed with negotiation.");

				startServerEncryption();
			} else replyClient(500, "Unknown command.");
		} else if(!cmd.compare("PBSZ", Qt::CaseInsensitive)) {
			if(useSsl && (s_proxySslMode == FtpServer::Auto || s_proxySslMode == FtpServer::Explicit))
				dispatchServerCommand(msg);
			else
				replyClient(200, QString("PBSZ set to %1.").arg(msg.section(' ', 1, 1)));
		} else if(!cmd.compare("PROT", Qt::CaseInsensitive)) {
			if(useSsl && (s_proxySslMode == FtpServer::Auto || s_proxySslMode == FtpServer::Explicit))
				dispatchServerCommand(msg);
			else
				replyClient(200, QString("PROT now %1").arg(msg.section(' ', 1, 1)));

			if(msg.section(' ', 1, 1) == "P")
				useSslOnData = true;
		} else if(!cmd.compare("QUIT", Qt::CaseInsensitive)) {
			if(targetServer)
			{
				targetServer->close();
				delete targetServer;
				targetServer = 0;
			}
			replyClient(221, "Goodbye.");
			close();

			if(dataTransfer)
				delete dataTransfer;
		} else {
			if(targetServer)
			{
				//targetServer->write(rawMsg);
				dispatchServerCommand(msg);

			} else
				replyClient(530, "Please login with USER and PASS.");
		}
	}
}

void FtpConnection::forwardTargetServerReply()
{
	while(targetServer->bytesAvailable())
	{
		QByteArray rawMsg = targetServer->readLine();
		QString strMsg(rawMsg);
		strMsg = strMsg.trimmed();

		if(serverCommands.isEmpty())
		{
			qDebug() << "Direct forward" << strMsg;
			write(rawMsg);
			continue;
		}

		qDebug() << "Received response from server" << strMsg;

		if(commandSent)
		{
			QString rc = strMsg.section(' ', 0, 0);
			QString msg = strMsg.section(' ', 1, -1);
			FtpCommand *fc = serverCommands.head();
			bool dequeue = true;

			switch(fc->type)
			{
			case Greetings: {
				if(rc == "220")
				{
					qDebug() << "Greetings accepted, continue";
					waitForCommand = false;

					if(s_proxySslMode == FtpServer::Explicit || (s_proxySslMode == FtpServer::Auto && useSsl))
						enqueueServerCommand(Auth);

					enqueueServerCommand(User);
				} else if(rc == "120") {
					dequeue = false;
				} else {
					write(rawMsg);
				}

				break;
			}

			case Auth: {
				if(rc == "234")
				{
					qDebug() << "TLS negotiation";

					waitForCommand = true;

					targetServer->setProtocol(QSsl::TlsV1);
					targetServer->setPeerVerifyMode(QSslSocket::VerifyNone);
					targetServer->startClientEncryption();
				}

				break;
			}

			case User: {
				if(rc == "331")
				{
					userCmdSent = true;
					replyClient(331, "Please specify the password.");

					//enqueueServerCommand(Pass);
					if(!password.isEmpty())
						enqueueServerCommand(Pass, password);
				} else {
					userName.clear();
					replyClient(strMsg);
				}

				break;
			}

			case Pass: {
				replyClient(strMsg);

				if(rc == "230")
				{
					loggedOnServer = true;

					if(!useSsl && s_proxySslMode == FtpServer::Explicit)
					{
						enqueueServerCommand(Pbsz);
						enqueueServerCommand(Prot);
					}
				}

				break;
			}

			case Pbsz: {
				break;
			}

			case Prot: {
				break;
			}

			case Port: {
				if(!(fc->flags & NoForwardResponse))
					replyClient(rc.toInt(), msg);
				break;
			}

			case Pasv: {
				QPair<QHostAddress, quint16> addr = decodeHostAndPort(msg);

				engagePassiveDataConnection(addr.first, addr.second, fc->flags);

//				qDebug() << "Internal FTP server is listening on" << addr.first << addr.second;

				if(fc->flags & ActiveToPassiveTranslation)
					replyClient(200, "PORT command successful.");
				else
					replyClient(227, QString("Entering Passive Mode (%1)").arg(encodeHostAndPort(dataTransfer->clientServerAddress(), dataTransfer->clientServerPort())));

				break;
			}

			case Eprt: {
				if(!(fc->flags & NoForwardResponse))
					replyClient(rc.toInt(), msg);
				break;
			}

			case Epsv: {
				QPair<QHostAddress, quint16> addr = decodeExtendedHostAndPort(msg);

				engagePassiveDataConnection(targetServer->peerAddress(), addr.second, fc->flags);

//				qDebug() << "Internal FTP server is listening on" << addr.first << addr.second;

				if(fc->flags & ActiveToPassiveTranslation)
					replyClient(200, "EPRT command successful.");
				else
					replyClient(229, QString("Entering Extended Passive Mode (%1)").arg(encodeExtendedHostAndPort(QHostAddress(), dataTransfer->clientServerPort())));

				break;
			}

			default:
				replyClient(strMsg);
				break;
			}

			if(dequeue)
				delete serverCommands.dequeue();

			if(serverCommands.isEmpty())
			{
				qDebug() << "Send queue empty";
				//continue;
			}

			commandSent = false;
		}

		dispatchServerCommand();
	}
}

void FtpConnection::enqueueServerCommand(FtpCommandType cmd, QString arg, FtpCommandFlags flags)
{
	FtpCommand *fc = new FtpCommand;
	fc->type = cmd;
	fc->str = arg;
	fc->flags = flags;

	serverCommands.enqueue(fc);
}

void FtpConnection::enqueueServerCommand(QString cmd, QString arg)
{
	enqueueServerCommand(Raw, QString("%1 %2").arg(cmd).arg(arg));
}

void FtpConnection::enqueueServerCommand(QString cmd)
{
	enqueueServerCommand(Raw, cmd);
}

void FtpConnection::dispatchServerCommand()
{
	if(commandSent || waitForCommand || serverCommands.isEmpty())
		return;

	QString cmd, arg;
	FtpCommand *fc = serverCommands.head();

	switch(fc->type)
	{
	case Auth:
		cmd = "AUTH TLS";
		break;
	case Pbsz:
		cmd = "PBSZ";
		arg = "0";
		break;
	case Prot:
		cmd = "PROT";
		arg = "P";
		break;
	case User:
		cmd = "USER";
		arg = userName;
		break;
	case Pass:
		cmd = "PASS";
		arg = password;
		break;
	case Port:
		cmd = "PORT";
		arg = fc->str;
		break;
	case Pasv:
		cmd = "PASV";
		break;
	case Eprt:
		cmd = "EPRT";
		arg = fc->str;
		break;
	case Epsv:
		cmd = "EPSV";
		break;
	case Raw:
		cmd = fc->str;
		break;
	default:return;
	}

	commandSent = true;

	if(arg.isEmpty())
		replyServer(cmd);
	else replyServer(cmd, arg);
}

void FtpConnection::dispatchServerCommand(FtpCommandType cmd, QString arg, FtpCommandFlags flags)
{
	enqueueServerCommand(cmd, arg, flags);
	dispatchServerCommand();
}

void FtpConnection::dispatchServerCommand(QString cmd, QString arg)
{
	enqueueServerCommand(cmd, arg);
	dispatchServerCommand();
}

void FtpConnection::dispatchServerCommand(QString cmd)
{
	enqueueServerCommand(cmd);
	dispatchServerCommand();
}

void FtpConnection::controlConnectionStateChange(QAbstractSocket::SocketState socketState)
{
	qDebug() << "Control connection state changed to" << socketState;

	if(socketState == QAbstractSocket::ConnectedState)
		sendGreetings();
}

void FtpConnection::controlConnectionEstablished()
{
	qDebug() << "Control connection established";
}

void FtpConnection::targetServerConnectionEncrypted()
{
	qDebug() << "TargetServer connection encrypted! :)";

	waitForCommand = false;
	dispatchServerCommand();
}

void FtpConnection::targetServerConnectionError(QAbstractSocket::SocketError error)
{
	qDebug() << "Error on control connection to target server:" << error;

	if(loggedOnServer)
	{
		replyClient(421, "Service not available.");
		disconnectFromHost();
	} else
		targetServerConnectionState = Failed;
}

void FtpConnection::engageActiveDataConnection(QHostAddress host, quint16 port)
{
	dataTransfer = new FtpDataTransfer(FtpDataTransfer::Active, FtpDataTransfer::Active, this);
	dataTransfer->setClient(host, port);
	dataTransfer->setServerListenAddress(targetServer->localAddress());

	dataTransfer->setClientReadBufferSize(s_readBufferSize);
	dataTransfer->setServerReadBufferSize(s_readBufferSize);

	connect(dataTransfer, SIGNAL(transferFinished()), this, SLOT(dataTransferFinished()));

	// FIXMEEEE forceFtps FIXME !!! if(useSsl) is wrong since ssl may be needed only between proxy and server
	dataTransfer->setUseSsl(
				useSslOnData,
				s_proxySslMode == FtpServer::Explicit || (useSslOnData && s_proxySslMode == FtpServer::Auto),
				sslCertificate,
				sslKey
	);

	dataTransfer->start();
}

void FtpConnection::engagePassiveDataConnection(QHostAddress host, quint16 port, FtpCommandFlags flags)
{
	if(flags & ActiveToPassiveTranslation)
	{
		dataTransfer = new FtpDataTransfer(FtpDataTransfer::Active, FtpDataTransfer::Passive, this);
		dataTransfer->setClient(lastPortAddr.first, lastPortAddr.second);
	} else {
		dataTransfer = new FtpDataTransfer(FtpDataTransfer::Passive, FtpDataTransfer::Passive, this);
		dataTransfer->setClientListenAddress(localAddress());
	}

	dataTransfer->setServer(host, port);

	dataTransfer->setClientReadBufferSize(s_readBufferSize);
	dataTransfer->setServerReadBufferSize(s_readBufferSize);

	dataTransfer->setUseSsl(useSslOnData,
				s_proxySslMode == FtpServer::Explicit || (useSslOnData && s_proxySslMode == FtpServer::Auto),
				sslCertificate,
				sslKey);

	connect(dataTransfer, SIGNAL(transferFinished()), this, SLOT(dataTransferFinished()));

	dataTransfer->start();

}

void FtpConnection::engagePassiveToActiveDataConnectionTranslation()
{
	dataTransfer = new FtpDataTransfer(FtpDataTransfer::Passive, FtpDataTransfer::Active, this);
	dataTransfer->setClientListenAddress(localAddress());
	dataTransfer->setServerListenAddress(targetServer->localAddress());

	dataTransfer->setClientReadBufferSize(s_readBufferSize);
	dataTransfer->setServerReadBufferSize(s_readBufferSize);

	dataTransfer->setUseSsl(useSslOnData,
				s_proxySslMode == FtpServer::Explicit || (useSslOnData && s_proxySslMode == FtpServer::Auto),
				sslCertificate,
				sslKey);

	connect(dataTransfer, SIGNAL(transferFinished()), this, SLOT(dataTransferFinished()));

	dataTransfer->start();
}

void FtpConnection::dataTransferFinished()
{
	dataTransfer->deleteLater();
	dataTransfer = 0;

	qDebug() << "Data transfer finished";
}

QPair<QHostAddress, quint16> FtpConnection::decodeHostAndPort(QString msg)
{
	QRegExp rx("(\\d{1,3}),(\\d{1,3}),(\\d{1,3}),(\\d{1,3}),(\\d{1,3}),(\\d{1,3})");

	if(rx.indexIn(msg) != -1)
	{
		QStringList parts = rx.capturedTexts();
		QString host = parts[1] + "." + parts[2] + "." + parts[3] + "." + parts[4];
		quint16 port = (parts[5].toUInt() << 8) + parts[6].toUInt();

		return QPair<QHostAddress, quint16>(QHostAddress(host), port);
	} else
		return QPair<QHostAddress, quint16>(QHostAddress(), 0);
}

QPair<QHostAddress, quint16> FtpConnection::decodeExtendedHostAndPort(QString msg)
{
//	QStringList arg = msg.split(' ');

//	if(arg.count() != 2)
//		return QPair<QHostAddress, quint16>(QHostAddress(), 0);

//	QStringList parts = arg[1].split('|');
//	qDebug() << "Parts = " << parts;

//	if(parts.count() != 5)
//		return QPair<QHostAddress, quint16>(QHostAddress(), 0);

//	return QPair<QHostAddress, quint16>(QHostAddress(parts[2]), parts[3].toInt());

	QRegExp rx("\\|(\\d?)\\|(.*)\\|(\\d+)\\|");

	if(rx.indexIn(msg) != -1)
		return QPair<QHostAddress, quint16>(QHostAddress(rx.cap(2)), rx.cap(3).toInt());
	return QPair<QHostAddress, quint16>(QHostAddress(), 0);
}

QString FtpConnection::encodeHostAndPort(QHostAddress addr, quint16 port)
{
	return QString("%1,%2,%3")
			.arg(addr.toString().replace(".", ","))
			.arg((port & 0xff00) >> 8)
			.arg(port & 0xff
	);
}

QString FtpConnection::encodeExtendedHostAndPort(QHostAddress addr, quint16 port)
{
	return QString("|%1|%2|%3|")
			.arg(addr.isNull() ? "" : (addr.protocol() == QAbstractSocket::IPv4Protocol ? "1" : "2"))
			.arg(addr.isNull() ? "" : addr.toString())
			.arg(port);
}
