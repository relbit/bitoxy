#ifndef LOGFORMATTER_H
#define LOGFORMATTER_H

#include "Logger.h"

class LogFormatter
{
public:
	LogFormatter();
	LogFormatter(Logger *logger);
	LogFormatter(QString format, Logger *logger);
	LogFormatter(const LogFormatter &other);
	LogFormatter& set(QString var, QString val);
	LogFormatter& set(QString var, int val);
	LogFormatter& set(QString var, quint64 val);
	void log(Logger::Level level);
	void log(Logger::Level level, QString msg);
	bool isValid() const;
	QString format();

private:
	Logger *m_logger;
	QString m_format;
};

#endif // LOGFORMATTER_H
