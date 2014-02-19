#ifndef ACCESSLOGMESSAGEGE_H
#define ACCESSLOGMESSAGEGE_H

#include "LogFormatter.h"

#include <QHostAddress>

class AccessLogMessage
{
public:
	enum Variable {
		CommandId=1,
		CommandString=2,
		StatusCode=4,
		StatusString=8,
		BytesSent=16,
		User=32,
		ClientAddr=64,
		ClientPort=128,
		ServerAddr=256,
		ServerPort=512,
		Proxy=1024,
		Encryption=2048
	};

	enum WaitReason {
		NoReason,
		ClientMissing,
		DataTransfer
	};

	AccessLogMessage();
	AccessLogMessage(LogFormatter *fmt);
	AccessLogMessage(const AccessLogMessage &other);
	void setFormatter(LogFormatter *fmt);
	bool hasClient() const;
	AccessLogMessage& client(QHostAddress host, quint16 port);
	AccessLogMessage& encrypted();
	AccessLogMessage& login(QString user);
	AccessLogMessage& proxyOn(QHostAddress host, quint16 port);
	AccessLogMessage& proxyOff();
	AccessLogMessage& cmd(quint64 id, QString msg);
	AccessLogMessage& sentBytes(quint64 bytes);
	AccessLogMessage& status(int code, QString msg);
	AccessLogMessage& status(QString code, QString msg);
	void setDataTransferActive(bool active, quint64 bytes = 0);
	void send();
	void reset();

private:
	LogFormatter *m_originalFormatter;
	LogFormatter m_currentFormatter;
	QString m_user;
	QHostAddress m_clientAddr;
	quint16 m_clientPort;
	QHostAddress m_serverAddr;
	quint16 m_serverPort;
	bool m_encrypted;
	bool m_data;
	int m_vars;
	QString m_waitingMsg;
	WaitReason m_waitReason;
};

#endif // ACCESSLOGMESSAGEGE_H
