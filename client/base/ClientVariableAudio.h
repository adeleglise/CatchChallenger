#ifndef CATCHCHALLENGER_CLIENTVARIABLEAUDIO_H
#define CATCHCHALLENGER_CLIENTVARIABLEAUDIO_H

#include <QObject>

#if ! defined(Q_OS_LINUX) && ! defined(Q_OS_MAC) && ! defined(Q_OS_WIN32)
#define CATCHCHALLENGER_NOAUDIO
#endif

#endif // CATCHCHALLENGER_CLIENTVARIABLEAUDIO_H
