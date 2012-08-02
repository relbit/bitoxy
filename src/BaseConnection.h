#ifndef BASECONNECTION_H
#define BASECONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>

class BaseConnection : public QSslSocket
{
	Q_OBJECT
public:
	explicit BaseConnection(QObject *parent = 0);
	virtual void applySettings(QHash<QString, QVariant> &settings);

signals:
	void disconnected(BaseConnection *connection);
	
public slots:

protected:
	QSslSocket *targetServer;

private slots:
	void resendDisconnected();
	
};

#endif // BASECONNECTION_H
