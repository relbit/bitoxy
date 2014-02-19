#include "BaseConnection.h"

BaseConnection::BaseConnection(QObject *parent) :
	QSslSocket(parent),
	targetServer(0),
	m_cmdCount(0)
{
	connect(this, SIGNAL(disconnected()), this, SLOT(resendDisconnected()));
}

void BaseConnection::applySettings(QHash<QString, QVariant> &settings)
{
	Q_UNUSED(settings);

	qDebug() << "BaseConnection settings applied";
}

void BaseConnection::setLogFormatter(LogFormatter &fmt)
{
	m_logFormatter = fmt;
	accessLog.setFormatter(&m_logFormatter);
}

void BaseConnection::resendDisconnected()
{
	emit disconnected(this);
}
