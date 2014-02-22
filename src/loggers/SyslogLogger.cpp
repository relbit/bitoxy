#include "SyslogLogger.h"

#include <QDebug>

SyslogHandler* SyslogHandler::m_instance = 0;

SyslogHandler::SyslogHandler(int facility)
	: m_refCount(0)
{
	qDebug() << "Opening connection to syslog with facility" << facility;

	openlog("bitoxy", 0, facility);
}

SyslogHandler::~SyslogHandler()
{
	closelog();
}

SyslogHandler* SyslogHandler::instance(int facility)
{
	if(!m_instance)
		m_instance = new SyslogHandler(facility);

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
	QString rawFacility = settings.value("Facility", "daemon").toString();
	int facility;
	int locals[8] = {
		LOG_LOCAL0,
		LOG_LOCAL1,
		LOG_LOCAL2,
		LOG_LOCAL3,
		LOG_LOCAL4,
		LOG_LOCAL5,
		LOG_LOCAL6,
		LOG_LOCAL7
	};

	if(!rawFacility.compare("daemon"))
	{
		facility = LOG_DAEMON;

	} else if(!rawFacility.compare("ftp")) {
		facility = LOG_FTP;

	} else if(rawFacility.startsWith("local")) {
		bool ok;
		int n = rawFacility.right(1).toInt(&ok);

		if(!ok || n < 0 || n > 7)
		{
			qWarning() << "Unsupported syslog facility" << rawFacility << ", using daemon instead";
			facility = LOG_DAEMON;
		} else
			facility = locals[n];

	} else if(!rawFacility.compare("user")) {
		facility = LOG_USER;

	} else {
		qWarning() << "Unsupported syslog facility" << rawFacility << ", using daemon instead";
		facility = LOG_DAEMON;
	}

	SyslogHandler::instance(facility)->registerLogger();
}

SyslogLogger::~SyslogLogger()
{
	SyslogHandler::instance()->release();
}

void SyslogLogger::log(Level level, QString& msg)
{
	SyslogHandler::instance()->log(level, msg);
}
