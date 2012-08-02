#ifndef BITOXY_H
#define BITOXY_H

#include <QObject>
#include <QList>

#include "BaseTcpServer.h"
#include "Worker.h"

void savePid(QString file, pid_t pid);

class Bitoxy : public QObject
{
	Q_OBJECT
public:
	explicit Bitoxy(QObject *parent = 0);
	~Bitoxy();
	bool init(QString config);
	
signals:
//	void incomingConnectionQueued();
	
public slots:

private:
	QList<Worker*> workers;

private slots:
	void processNewConnection(IncomingConnection connection);
	
};

#endif // BITOXY_H
