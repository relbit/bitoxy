#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>
#include <QDateTime>
#include <QDebug>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <signal.h>

#include "Bitoxy.h"
#include "Router.h"
#include "routers/StaticRouter.h"
#include "routers/SqlRouter.h"
#include "services/ftp/FtpServer.h"
#include "loggers/SyslogLogger.h"

QFile* Bitoxy::logFile = 0;
QMutex Bitoxy::mutex;
bool Bitoxy::m_debug = false;

Bitoxy::Bitoxy(QObject *parent) :
        QObject(parent)
{
	connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));
}

Bitoxy::~Bitoxy()
{
	finish();
}

bool Bitoxy::init(QString config)
{
	QSettings cfg(config, QSettings::IniFormat);

	if(cfg.status() == QSettings::AccessError)
	{
		qCritical() << "Cannot access config file" << config;
		return false;
	} else if(cfg.status() == QSettings::FormatError) {
		qCritical() << "Cannot parse config file" << config;
		return false;
	}

	QStringList groups = cfg.childGroups();
	QHash<QString, int> routers;

	QString user, group;
	uid_t uid = geteuid();
	gid_t gid = getegid();
	uid_t newUid;
	gid_t newGid;
	bool dropPrivileges = false;

	if(!uid || !gid)
	{
		user = cfg.value("User").toString();
		group = cfg.value("Group").toString();

		if(user.isEmpty() || group.isEmpty())
		{
			qWarning() << "WARNING: Running as root! U mad?";
		} else {
			struct passwd *pwd;
			struct group *grp;

			if((pwd = getpwnam(user.toAscii().data())) == NULL)
			{
				qWarning() << "User" << user << "does not exist.";
				return false;
			} else if((grp = getgrnam(group.toAscii().data())) == NULL) {
				qWarning() << "Group" << group << "does not exist.";
				return false;
			}

			if(!pwd->pw_uid || !grp->gr_gid)
			{
				qWarning() << "WARNING: Running as root! You're just mad.";
			}

			dropPrivileges = true;
			newUid = pwd->pw_uid;
			newGid = grp->gr_gid;
		}
	}

	foreach(QString group, groups)
	{
		QStringList type = group.split(':');

		if(type.count() < 2)
		{
			qDebug() << "Invalid group" << group;
			continue;
		}

		cfg.beginGroup(group);

		if(!type[0].compare("service", Qt::CaseInsensitive))
		{
			BaseTcpServer *server = 0;
			QStringList addrs;
			QHostAddress hostAddr;
			quint16 port;
			QString router;
			QString accessLog;
			int routerId = 0;

			addrs = cfg.value("Host").toStringList();

			foreach(QString addr, addrs)
			{
				if(!hostAddr.setAddress(addr))
				{
					qWarning() << "Invalid address" << addr;
					continue;
				}

				if(!(port = cfg.value("Port", 0).toInt()))
				{
					qWarning() << "Invalid port";
					continue;
				}

				if((router = cfg.value("Router").toString()).isEmpty())
				{
					qWarning() << "No router specified for service" << type[1];
					continue;
				} else {
					routerId = routers[router];
				}

				if(!(accessLog = cfg.value("AccessLog").toString()).isEmpty() && !loggers.contains(accessLog))
				{
					qWarning() << "Access log" << accessLog << "not found";
					continue;
				}

#ifdef SERVICE_FTP
				if(!type[1].compare("ftp", Qt::CaseInsensitive))
				{
					server = new FtpServer(cfg, this);
					if(server->listen(hostAddr, port))
						qDebug() << "FTP server: Listening on" << QString("%1:%2").arg(addr).arg(port);
					else
						qDebug() << "FTP server: Unable to listen on" << QString("%1:%2").arg(addr).arg(port) <<":" << server->serverError();
				}
#endif

				if(server)
				{
					connect(server, SIGNAL(newConnection(IncomingConnection)),
						this, SLOT(processNewConnection(IncomingConnection))
					);

					server->setRouter(routerId);

					if(!accessLog.isEmpty())
					{
						LogFormatter fmt(loggers[accessLog]);
						fmt.set("type", "access");

						server->setLogFormatter(fmt);
					}
				}
			}

		} else if(!type[0].compare("router", Qt::CaseInsensitive)) {
			Router *router = 0;

			if(!type[1].compare("Static", Qt::CaseInsensitive))
			{
				router = new StaticRouter(cfg);

			} else if(!type[1].compare("Sql", Qt::CaseInsensitive)) {
				router = new SqlRouter(cfg);
			}

			if(router)
			{
				qDebug() << "Registered router" << type[1];
				routers[ type[2] ] = Router::registerRouter(router);
			}

		} else if(!type[0].compare("log", Qt::CaseInsensitive)) {
			if(!type[1].compare("syslog", Qt::CaseInsensitive))
			{
				loggers[type[2]] = new SyslogLogger(cfg, this);

				qDebug() << "Registered logger" << type[2];
			}
		}

		cfg.endGroup();
	}

	if(dropPrivileges)
	{
		if(setgid(newGid) == -1)
		{
			qWarning() << "Unable to setgid" << newGid;
			return false;
		} else if(setuid(newUid) == -1) {
			qWarning() << "Unable to setuid" << newUid;
			return false;
		} else {
			qDebug() << "Switched to user" << user << "and group" << group;
		}
	}

	bool ok;
	int threadCount = cfg.value("Threads", QThread::idealThreadCount()).toInt(&ok);

	if(!ok || threadCount == -1)
	{
		threadCount = 2;
		qDebug() << "Thread count cannot be determined, spawning two threads";
	} else if(!threadCount || threadCount < 0) {
		threadCount = 1;
	}

	qDebug() << "Spawning" << threadCount << "workers";

	for(int i = 0; i < threadCount; i++)
	{
		QThread *thread = new QThread(this);

		Worker *w = new Worker();
		w->moveToThread(thread);

//		w->start();

		//connect(this, SIGNAL(incomingConnectionQueued()), w, SLOT(processIncomingConnections()));

		workers << w;

		thread->start();
	}

	return true;
}

void Bitoxy::finish()
{
	qDebug() << "Stopping workers";

	foreach(Worker *w, workers)
		w->thread()->quit();

	qDebug() << "Waiting for workers to quit...";

	foreach(Worker *w, workers)
	{
		w->thread()->wait();
		delete w;
	}

	QList<Router*> routers = Router::routers();
	qDeleteAll(routers);

	qDeleteAll(loggers);
}

void Bitoxy::installLogFileHandler(QString &logFilePath)
{
	if(logFilePath.isEmpty())
	{
		logFile = new QFile(this);

		if(!logFile->open(stdout, QIODevice::WriteOnly))
		{
			qWarning() << "Unable to open stdout for logging:" << logFile->errorString();
			return;
		}

	} else {
		logFile = new QFile(logFilePath, this);

		if(!logFile->open(QIODevice::Append))
		{
			qWarning() << "Unable to open log file" << logFilePath << "in append mode:" << logFile->errorString();
			return;
		}
	}

	qInstallMsgHandler(Bitoxy::logFileHandler);
}

void Bitoxy::logFileHandler(QtMsgType type, const char *msg)
{
	QMutexLocker locker(&mutex);

	QTextStream out(logFile);
	QString str = QString("[%1 %2]: %3\n").arg(QDateTime::currentDateTime().toString());

	switch (type)
	{
	case QtDebugMsg:
		if(Bitoxy::showDebug())
			out << str.arg("D").arg(msg);
		break;
	case QtWarningMsg:
		out << str.arg("WARNING").arg(msg);
		break;
	case QtCriticalMsg:
		out << str.arg("CRITICAL").arg(msg);
		break;
	case QtFatalMsg:
		out << str.arg("FATAL").arg(msg);
		abort();
	}
}

void Bitoxy::setDebug(bool enable)
{
	m_debug = enable;
}

bool Bitoxy::showDebug()
{
	return m_debug;
}

void Bitoxy::gracefullyExit(int sig)
{
	Q_UNUSED(sig);

	switch(sig)
	{
	case SIGTERM:
		qDebug() << "Received SIGTERM";
		break;
	case SIGINT:
		qDebug() << "Received SIGINT";
		break;
	default:
		qDebug() << "Received unhandled signal" << sig;
	}

	qDebug() << "Gracefully exit";
	qApp->quit();
}

void Bitoxy::processNewConnection(IncomingConnection connection)
{
	int workersCnt = workers.count();

	if(workersCnt == 1)
	{
		qDebug() << "Client assigned to first worker";
		workers.first()->addConnection(connection);
	}

	int minConnections = workers.first()->connectionCount();
	int index = 0;

	qDebug() << "Worker" << 0 << ":" << minConnections << "connections";

	for(int i = 1; i < workersCnt; i++)
	{
		int cnt = workers.at(i)->connectionCount();

		if(cnt < minConnections)
		{
			minConnections = cnt;
			index = i;
		}

		qDebug() << "Worker" << i << ":" << cnt << "connections";
	}

	qDebug() << "Client assigned to worker" << index;
	//workers.at(index)->addConnection(connection);

//	emit incomingConnectionQueued();

	connection.logFormatter.set("worker", index);

	QMetaObject::invokeMethod(workers.at(index), "addConnection", Qt::QueuedConnection, Q_ARG(IncomingConnection, connection));
}

void Bitoxy::aboutToQuit()
{
	qDebug() << "Bitoxy exiting at" << QDateTime::currentDateTime().toString();
}

void savePid(QString file, pid_t pid)
{
	QFile f(file);

	if(!f.open(QIODevice::WriteOnly))
	{
		qWarning() << "Unable to open file to write PID:" << f.errorString();
		return;
	}

	QTextStream stream(&f);
	stream << pid;
	f.close();
}

void help()
{
	QTextStream out(stdout);
	out << "Usage: bitoxy [-c CONFIG] [-d] [-p PIDFILE] [-g]\n";
	out << "\nOptions:\n";
	out << "    -c, --config=FILE    Specify config file, defaults to /etc/bitoxy.conf\n";
	out << "    -d, --daemon         Daemonize, by default stay on foreground\n";
	out << "    -g, --debug          Show debug messages, only errors are printed if not enabled\n";
	out << "    -p, --pidfile=FILE   Save PID to file\n";
	out << "    -l, --logfile=FILE   Redirect output to file\n";
	out << "    -h, --help           Show this message\n";
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	Bitoxy bit;
	int rc;

	QString config = "/etc/bitoxy.conf";
	QString pidFile;
	QString logFilePath;
	int daemon = 0;

	int c;
	int option_index = 0;

	static struct option long_options[] = {
		{"config",  required_argument, 0,      'c'},
		{"daemon",  no_argument,       &daemon, 1 },
		{"debug",   no_argument,       0,      'g'},
		{"pidfile", required_argument, 0,      'p'},
		{"logfile", required_argument, 0,      'l'},
		{"help",    no_argument,       0,      'h'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "c:dgp:l:h", long_options, &option_index)) != -1)
	{
		switch(c)
		{
		case 0:
			break;
		case 'c':
			config = optarg;
			break;
		case 'd':
			daemon = 1;
			break;
		case 'g':
			bit.setDebug(true);
			break;
		case 'p':
			pidFile = optarg;
			break;
		case 'l':
			logFilePath = optarg;
			break;
		case 'h':
			bit.installLogFileHandler(logFilePath);
			help();
			return 0;
		case '?':
			return 1;
		default:
			qDebug() << "Unknown option" << (char)c;
			return 1;
		}
	}

	bit.installLogFileHandler(logFilePath);

	qDebug() << "Bitoxy starting";

	if(daemon)
	{
		pid_t pid = fork();

		if(pid)
		{
			qDebug() << "Daemonize...";

			if(!pidFile.isEmpty())
			{
				qDebug() << "Saving PID to" << pidFile;
				savePid(pidFile, pid);
			}

			return 0;

		} else if(pid == -1) {
			qDebug() << "Unable to fork";
		}

	} else if(!pidFile.isEmpty()) {
		savePid(pidFile, getpid());
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = Bitoxy::gracefullyExit;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	qDebug() << "Using config" << config;

	if(bit.init(config))
	{
		rc = a.exec();

	} else {
		qWarning() << "Bitoxy init failed, exiting.";
		return 2;
	}

	return rc;
}
