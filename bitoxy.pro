#-------------------------------------------------
#
# Project created by QtCreator 2012-07-07T15:37:09
#
#-------------------------------------------------

QT       += core network sql
QT       -= gui

TARGET = bitoxy
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

VPATH += ./src

SOURCES += src/Bitoxy.cpp \
    src/BaseTcpServer.cpp \
    src/BaseConnection.cpp \
    src/Worker.cpp \
    src/Router.cpp \
    src/routers/StaticRouter.cpp \
    src/routers/SqlRouter.cpp \
    src/services/ftp/FtpServer.cpp \
    src/services/ftp/FtpConnection.cpp \
    src/services/ftp/FtpDataTransfer.cpp \
    src/services/ftp/FtpDataTransferServer.cpp \
    src/Logger.cpp \
    src/loggers/SyslogLogger.cpp \
    src/LogFormatter.cpp \
    src/AccessLogMessage.cpp

HEADERS += \
    src/Bitoxy.h \
    src/BaseTcpServer.h \
    src/BaseConnection.h \
    src/Worker.h \
    src/Router.h \
    src/routers/StaticRouter.h \
    src/routers/SqlRouter.h \
    src/services/ftp/FtpServer.h \
    src/services/ftp/FtpConnection.h \
    src/services/ftp/FtpDataTransfer.h \
    src/services/ftp/FtpDataTransferServer.h \
    src/Logger.h \
    src/loggers/SyslogLogger.h \
    src/LogFormatter.h \
    src/AccessLogMessage.h

OTHER_FILES += \
    bitoxy.conf
