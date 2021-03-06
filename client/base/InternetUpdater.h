#ifndef INTERNETUPDATER_H
#define INTERNETUPDATER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class InternetUpdater : public QObject
{
    Q_OBJECT
public:
    explicit InternetUpdater();
    static InternetUpdater *internetUpdater;
    static QString getText(const QString &version);
    #if defined(_WIN32) || defined(Q_OS_MAC)
    static QString GetOSDisplayString();
    #endif
signals:
    void newUpdate(const QString &version) const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
    bool versionIsNewer(const QString &version);
};

#endif // INTERNETUPDATER_H
