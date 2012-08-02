#include "BaseConnection.h"

BaseConnection::BaseConnection(QObject *parent) :
	QSslSocket(parent),
	targetServer(0)
{
	connect(this, SIGNAL(disconnected()), this, SLOT(resendDisconnected()));
}

void BaseConnection::applySettings(QHash<QString, QVariant> &settings)
{
	qDebug() << "BaseConnection settings applied";
}

void BaseConnection::resendDisconnected()
{
	emit disconnected(this);
}
