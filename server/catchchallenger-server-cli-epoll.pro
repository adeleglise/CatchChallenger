include(catchchallenger-server.pri)
include(../general/general.pri)

QT       -= gui widgets

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"

DEFINES += SERVERNOBUFFER
DEFINES += SERVERNOSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT

#LIBS += -lssl -lcrypto

CONFIG += c++11

TARGET = catchchallenger-server-cli-epoll
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main-epoll.cpp \
    epoll/EpollSocket.cpp \
    epoll/EpollClient.cpp \
    epoll/EpollServer.cpp \
    epoll/EpollSslClient.cpp \
    epoll/EpollSslServer.cpp \
    epoll/Epoll.cpp \
    epoll/EpollTimer.cpp \
    epoll/db/EpollPostgresql.cpp \
    epoll/timer/TimerDisplayEventBySeconds.cpp \
    epoll/timer/TimerCityCapture.cpp \
    epoll/timer/TimerSendInsertMoveRemove.cpp \
    epoll/timer/TimerPositionSync.cpp \
    epoll/timer/TimerDdos.cpp \
    NormalServerGlobal.cpp

HEADERS += epoll/EpollSocket.h \
    epoll/EpollClient.h \
    epoll/EpollServer.h \
    epoll/EpollSslClient.h \
    epoll/EpollSslServer.h \
    epoll/Epoll.h \
    epoll/BaseClassSwitch.h \
    epoll/EpollTimer.h \
    epoll/db/EpollPostgresql.h \
    epoll/timer/TimerDisplayEventBySeconds.h \
    epoll/timer/TimerCityCapture.h \
    epoll/timer/TimerPositionSync.h \
    epoll/timer/TimerSendInsertMoveRemove.h \
    epoll/timer/TimerDdos.h \
    NormalServerGlobal.h
