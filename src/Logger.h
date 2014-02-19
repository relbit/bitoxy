#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QSettings>

class Logger : public QObject
{
	Q_OBJECT
public:
	enum Level {
		Emergency,
		Alert,
		Critical,
		Error,
		Warning,
		Notice,
		Info,
		Debug
	};

	explicit Logger(QSettings &settings, QObject *parent = 0);
	virtual void log(Level level, QString& msg);
	QString format();

protected:
	QString m_format;

	QString levelToString(Level level);

};

#endif // LOGGER_H
