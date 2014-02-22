#include "AccessLogMessage.h"

#include <QDateTime>

AccessLogMessage::AccessLogMessage()
{
	m_clientPort = 0;
	m_serverPort = 0;
	m_vars = 0;
	m_encrypted = false;
}

AccessLogMessage::AccessLogMessage(LogFormatter *fmt)
{
	setFormatter(fmt);
}

AccessLogMessage::AccessLogMessage(const AccessLogMessage &other)
{
	m_originalFormatter = other.m_originalFormatter;
	m_currentFormatter = other.m_currentFormatter;
	m_user = other.m_user;
	m_clientAddr = other.m_clientAddr;
	m_clientPort = other.m_clientPort;
	m_serverAddr = other.m_serverAddr;
	m_serverPort = other.m_serverPort;
	m_vars = other.m_vars;
}

void AccessLogMessage::setFormatter(LogFormatter *fmt)
{
	m_originalFormatter = fmt;
	m_currentFormatter = *m_originalFormatter;
}

bool AccessLogMessage::hasClient() const
{
	return m_clientPort > 0;
}

AccessLogMessage& AccessLogMessage::client(QHostAddress host, quint16 port)
{
	m_clientAddr = host;
	m_clientPort = port;

	if(m_waitReason == ClientMissing)
	{
		LogFormatter f(m_waitingMsg, 0);
		f.set("client_ip", host.toString()).set("client_port", port);
		m_currentFormatter.log(Logger::Info, f.format());

		m_waitReason = NoReason;
		m_waitingMsg.clear();
	}

	return *this;
}

AccessLogMessage& AccessLogMessage::encrypted()
{
	m_encrypted = true;

	return *this;
}

AccessLogMessage& AccessLogMessage::login(QString user)
{
	m_user = user;

	return *this;
}

AccessLogMessage& AccessLogMessage::proxyOn(QHostAddress host, quint16 port)
{
	m_serverAddr = host;
	m_serverPort = port;

	return *this;
}

AccessLogMessage& AccessLogMessage::proxyOff()
{
	m_serverAddr.clear();
	m_serverPort = 0;

	return *this;
}

AccessLogMessage& AccessLogMessage::cmd(quint64 id, QString msg)
{
	m_currentFormatter.set("cmd_id", id).set("cmd_str", msg);

	m_vars |= CommandId | CommandString;

	return *this;
}

AccessLogMessage& AccessLogMessage::status(int code, QString msg)
{
	m_currentFormatter.set("status_code", code).set("status_str", msg);

	m_vars |= StatusCode | StatusString;

	return *this;
}

AccessLogMessage& AccessLogMessage::status(QString code, QString msg)
{
	m_currentFormatter.set("status_code", code).set("status_str", msg);

	m_vars |= StatusCode | StatusString;

	return *this;
}

void AccessLogMessage::setDataTransferActive(bool active, quint64 bytes)
{
	if(m_data && !active && m_waitReason == DataTransfer && bytes > 0)
	{
		LogFormatter f(m_waitingMsg, 0);
		f.set("bytes_sent", bytes);
		m_currentFormatter.log(Logger::Info, f.format());

		m_waitReason = NoReason;
		m_waitingMsg.clear();
	}

	m_data = active;
}

void AccessLogMessage::send()
{
	if((m_vars & CommandId) != CommandId)
		m_currentFormatter.set("cmd_id", "-");

	if((m_vars & CommandString) != CommandString)
		m_currentFormatter.set("cmd_str", "");

	if((m_vars & StatusCode) != StatusCode)
		m_currentFormatter.set("status_code", "");

	if((m_vars & StatusString) != StatusString)
		m_currentFormatter.set("status_str", "");

	if(!m_data && (m_vars & BytesSent) != BytesSent)
		m_currentFormatter.set("bytes_sent", 0);

	m_currentFormatter.set("user", m_user.isEmpty() ? "-" : m_user);

	if(!m_clientAddr.isNull())
	{
		m_currentFormatter.set("client_ip", m_clientAddr.toString());
		m_currentFormatter.set("client_port", m_clientPort);
	}

	m_currentFormatter.set("server_ip", m_serverAddr.isNull() ? "none" : m_serverAddr.toString());
	m_currentFormatter.set("server_port", m_serverPort);

	m_currentFormatter.set("proxy", m_serverPort > 0 ? "p" : "-");
	m_currentFormatter.set("encryption", m_encrypted ? "e" : "-");

	m_currentFormatter.set("datetime", QDateTime::currentDateTime().toString());

	if(m_clientAddr.isNull())
	{
		m_waitingMsg = m_currentFormatter.format();
		m_waitReason = ClientMissing;

	} else if(m_data) {
		if(m_waitReason == DataTransfer)
		{
			LogFormatter f(m_waitingMsg, 0);
			f.set("bytes_sent", 0);

			m_currentFormatter.log(Logger::Info, f.format());
		}

		m_waitingMsg = m_currentFormatter.format();
		m_waitReason = DataTransfer;

	} else {
		m_currentFormatter.log(Logger::Info);
	}

	reset();
}

void AccessLogMessage::reset()
{
	m_currentFormatter = *m_originalFormatter;
	m_vars = 0;
}
