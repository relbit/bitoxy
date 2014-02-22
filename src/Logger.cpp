#include "Logger.h"

#include <QDebug>

Logger::Logger(QSettings &settings, QObject *parent) :
        QObject(parent)
{
	m_format = settings.value("Format").toString();
}

Logger::~Logger()
{

}

void Logger::log(Level level, QString &msg)
{
	qDebug() << "[" << levelToString(level) << "] " << msg;
}

QString Logger::format()
{
	return m_format;
}

QString Logger::levelToString(Level level)
{
	switch(level)
	{
	case Emergency:
		return "EMERGENCY";
	case Alert:
		return "ALERT";
	case Critical:
		return "CRITICAL";
	case Error:
		return "ERROR";
	case Warning:
		return "WARNING";
	case Notice:
		return "NOTICE";
	case Info:
		return "INFO";
	case Debug:
		return "DEBUG";
	default:
		Q_ASSERT_X(0, "base logger", "unsupported log level");
		return "UNDEFINED";
	}
}
