#ifndef SYSLOGLOGGER_H
#define SYSLOGLOGGER_H

#include "../Logger.h"
#include <syslog.h>

class SyslogHandler
{
public:
	static SyslogHandler* instance(int facility = LOG_DAEMON);
	void log(Logger::Level level, QString &msg);
	void registerLogger();
	void release();

private:
	static SyslogHandler *m_instance;
	int m_refCount;

	SyslogHandler(int facility);
	~SyslogHandler();
	int translateLogLevel(Logger::Level level);
};

class SyslogLogger : public Logger
{
	Q_OBJECT
public:
	explicit SyslogLogger(QSettings &settings, QObject *parent = 0);
	virtual ~SyslogLogger();
	void log(Level level, QString& msg);
};

#endif // SYSLOGLOGGER_H
