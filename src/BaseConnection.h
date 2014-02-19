#ifndef BASECONNECTION_H
#define BASECONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>

#include "LogFormatter.h"
#include "AccessLogMessage.h"

class BaseConnection : public QSslSocket
{
	Q_OBJECT
public:
	explicit BaseConnection(QObject *parent = 0);
	virtual void applySettings(QHash<QString, QVariant> &settings);
	void setLogFormatter(LogFormatter &fmt);

signals:
	void disconnected(BaseConnection *connection);
	
public slots:

protected:
	QSslSocket *targetServer;
	LogFormatter m_logFormatter;
	AccessLogMessage accessLog;
	quint64 m_cmdCount;

private slots:
	void resendDisconnected();
	
};

#endif // BASECONNECTION_H
