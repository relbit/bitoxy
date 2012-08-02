#include "FtpServer.h"
#include "FtpConnection.h"

FtpServer::FtpServer(QSettings &settings, QObject *parent) :
	BaseTcpServer(settings, parent),
	s_forceSsl(settings.value("ForceSsl", false).toBool()),
	s_proxyActiveMode(settings.value("ProxyActiveMode", false).toBool()),
	s_readBufferSize(settings.value("ReadBufferSize", 32768).toInt())
{
	QString ssl = settings.value("Ssl").toString();

	if(!ssl.compare("explicit"))
		s_ssl = FtpServer::Explicit;
	else s_ssl = FtpServer::None;

	QString proxySslMode = settings.value("ProxySslMode").toString();

	if(!proxySslMode.compare("explicit"))
		s_proxySslMode = FtpServer::Explicit;
	else if(!proxySslMode.compare("auto"))
		s_proxySslMode = FtpServer::Auto;
	else s_proxySslMode = FtpServer::None;

	QString proxyMode = settings.value("ProxyMode").toString();

	if(!proxyMode.compare("active"))
		s_proxyMode = FtpDataTransfer::Active;
	else if(!proxyMode.compare("passive"))
		s_proxyMode = FtpDataTransfer::Passive;
	else s_proxyMode = FtpDataTransfer::Auto;
}

void FtpServer::incomingConnection(int socketDescriptor)
{
//	FtpConnection *con = new FtpConnection(this);
//	con->setSocketDescriptor(socketDescriptor);

//	connect(con, SIGNAL(disconnected()), con, SLOT(deleteLater()));

//	qDebug() << "Client connected" << con->peerAddress();

//	con->sayHi();

	IncomingConnection con;

	con.serviceType = BaseTcpServer::FtpService;
	con.socketDescriptor = socketDescriptor;

	con.settings["Ssl"] = s_ssl;
	con.settings["ForceSsl"] = s_forceSsl;
	con.settings["ProxySslMode"] = s_proxySslMode;
	con.settings["ProxyMode"] = s_proxyMode;
	con.settings["ProxyActiveMode"] = s_proxyActiveMode;
	con.settings["Router"] = router;
	con.settings["ReadBufferSize"] = s_readBufferSize;

	emit newConnection(con);
}
