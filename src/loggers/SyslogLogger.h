#ifndef SYSLOGLOGGER_H
#define SYSLOGLOGGER_H

#include "../Logger.h"

class SyslogHandler
{
public:
	static SyslogHandler* instance();
	void log(Logger::Level level, QString &msg);

private:
	static SyslogHandler *m_instance;

	SyslogHandler();
	~SyslogHandler();
	int translateLogLevel(Logger::Level level);
};

class SyslogLogger : public Logger
{
	Q_OBJECT
public:
	explicit SyslogLogger(QSettings &settings, QObject *parent = 0);
	void log(Level level, QString& msg);

};

#endif // SYSLOGLOGGER_H
