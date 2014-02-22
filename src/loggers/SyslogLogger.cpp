#include "SyslogLogger.h"

#include <QDebug>
#include <syslog.h>

SyslogHandler* SyslogHandler::m_instance = 0;

SyslogHandler::SyslogHandler()
	: m_refCount(0)
{
	openlog("bitoxy", 0, LOG_DAEMON);
}

SyslogHandler::~SyslogHandler()
{
	closelog();
}

SyslogHandler* SyslogHandler::instance()
{
	if(!m_instance)
		m_instance = new SyslogHandler();

	return m_instance;
}

void SyslogHandler::log(Logger::Level level, QString &msg)
{
	QByteArray ba = msg.toLatin1();
	const char *str = ba.data();

	syslog(translateLogLevel(level), "%s", str);
}

void SyslogHandler::registerLogger()
{
	m_refCount++;
}

void SyslogHandler::release()
{
	if(--m_refCount == 0)
	{
		delete this;
		m_instance = 0;

		qDebug() << "Freeing syslog handler";
	}
}

int SyslogHandler::translateLogLevel(Logger::Level level)
{
	switch(level)
	{
	case Logger::Emergency:
		return LOG_EMERG;
	case Logger::Alert:
		return LOG_ALERT;
	case Logger::Critical:
		return LOG_CRIT;
	case Logger::Error:
		return LOG_ERR;
	case Logger::Warning:
		return LOG_WARNING;
	case Logger::Notice:
		return LOG_NOTICE;
	case Logger::Info:
		return LOG_INFO;
	case Logger::Debug:
		return LOG_DEBUG;
	default:
		Q_ASSERT_X(0, "syslog handler", "unsupported log level");
		return 0;
	}
}

SyslogLogger::SyslogLogger(QSettings &settings, QObject *parent) :
	Logger(settings, parent)
{
	SyslogHandler::instance()->registerLogger();
}

SyslogLogger::~SyslogLogger()
{
	SyslogHandler::instance()->release();
}

void SyslogLogger::log(Level level, QString& msg)
{
	SyslogHandler::instance()->log(level, msg);
}
