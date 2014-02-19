#include "LogFormatter.h"

#include <QDebug>

LogFormatter::LogFormatter()
	: m_logger(0)
{

}

LogFormatter::LogFormatter(Logger *logger)
	: m_logger(logger)
{
	m_format = logger->format();
}

LogFormatter::LogFormatter(QString format, Logger *logger)
	: m_logger(logger),
	  m_format(format)
{

}

LogFormatter::LogFormatter(const LogFormatter &other)
{
	m_logger = other.m_logger;
	m_format = other.m_format;
}

LogFormatter& LogFormatter::set(QString var, QString val)
{
	m_format.replace("$" + var, val);

	return *this;
}

LogFormatter& LogFormatter::set(QString var, int val)
{
	return set(var, QString::number(val));
}

LogFormatter& LogFormatter::set(QString var, quint64 val)
{
	return set(var, QString::number(val));
}

void LogFormatter::log(Logger::Level level)
{
	if(!isValid())
		return;

	m_logger->log(level, m_format);
}

void LogFormatter::log(Logger::Level level, QString msg)
{
	if(!isValid())
		return;

	m_logger->log(level, msg);
}

bool LogFormatter::isValid() const
{
	return m_logger;
}

QString LogFormatter::format()
{
	return m_format;
}
