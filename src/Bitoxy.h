#ifndef BITOXY_H
#define BITOXY_H

#include <QObject>
#include <QList>
#include <QFile>
#include <QMutex>

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
	void finish();
	void installLogFileHandler(QString &logFilePath);
	static void logFileHandler(QtMsgType type, const char *msg);
	static void setDebug(bool enable);
	static bool showDebug();
	static void gracefullyExit(int sig);
	
signals:
//	void incomingConnectionQueued();
	
public slots:
	void aboutToQuit();

private:
	QList<Worker*> workers;
	QHash<QString, Logger*> loggers;
	static QFile *logFile;
	static QMutex mutex;
	static bool m_debug;

private slots:
	void processNewConnection(IncomingConnection connection);
	
};

#endif // BITOXY_H
