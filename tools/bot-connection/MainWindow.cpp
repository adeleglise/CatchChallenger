#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettings.h"
#include "../../general/base/FacilityLib.h"

#include <QNetworkProxy>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    ui->setupUi(this);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::doMove);
    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::doText);
    moveTimer.start(1000);
    textTimer.start(1000);
    number=1;
    numberOfBotConnected=0;
    numberOfSelectedCharacter=0;

    if(settings.contains("login"))
        ui->login->setText(settings.value("login").toString());
    if(settings.contains("pass"))
        ui->pass->setText(settings.value("pass").toString());
    if(settings.contains("host"))
        ui->host->setText(settings.value("host").toString());
    if(settings.contains("port"))
        ui->port->setValue(settings.value("port").toUInt());
    if(settings.contains("proxy"))
        ui->proxy->setText(settings.value("proxy").toString());
    if(settings.contains("proxyport"))
        ui->proxyport->setValue(settings.value("proxyport").toUInt());
    if(settings.contains("multipleConnexion"))
    {
        if(settings.value("multipleConnexion").toUInt()<2)
            ui->multipleConnexion->setChecked(false);
        else
        {
            ui->multipleConnexion->setChecked(true);
            ui->connexionCount->setValue(settings.value("multipleConnexion").toUInt());
        }
        if(settings.contains("connectBySeconds"))
            ui->connectBySeconds->setValue(settings.value("connectBySeconds").toUInt());
        if(settings.contains("maxDiffConnectedSelected"))
            ui->maxDiffConnectedSelected->setValue(settings.value("maxDiffConnectedSelected").toUInt());
    }
    if(settings.contains("autoCreateCharacter"))
        ui->autoCreateCharacter->setChecked(settings.value("autoCreateCharacter").toBool());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::disconnected()
{
    numberOfBotConnected--;
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
        return;

    if(!connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason)
    {
        ui->statusBar->showMessage(tr("Disconnected by the host"));
        ui->connect->setEnabled(true);
    }
    connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=false;
    if(connectedSocketToCatchChallengerClient[senderObject]->selectedCharacter==true)
    {
        connectedSocketToCatchChallengerClient[senderObject]->selectedCharacter=false;
        numberOfSelectedCharacter--;
        ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));
    }
}

void MainWindow::tryLink()
{
    numberOfBotConnected++;
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
        return;

    if(!ui->multipleConnexion->isChecked())
    {
        connectedSocketToCatchChallengerClient[senderObject]->login=ui->login->text();
        connectedSocketToCatchChallengerClient[senderObject]->api->startReadData();
        connectedSocketToCatchChallengerClient[senderObject]->api->sendProtocol();
        connectedSocketToCatchChallengerClient[senderObject]->api->tryLogin(ui->login->text(),ui->pass->text());
    }
    else
    {
        QString login=ui->login->text();
        QString pass=ui->pass->text();
        login.replace(QLatin1Literal("%NUMBER%"),QString::number(connectedSocketToCatchChallengerClient[senderObject]->number));
        pass.replace(QLatin1Literal("%NUMBER%"),QString::number(connectedSocketToCatchChallengerClient[senderObject]->number));
        connectedSocketToCatchChallengerClient[senderObject]->login=login;
        connectedSocketToCatchChallengerClient[senderObject]->api->startReadData();
        connectedSocketToCatchChallengerClient[senderObject]->api->sendProtocol();
        connectedSocketToCatchChallengerClient[senderObject]->api->tryLogin(login,pass);
    }
}

void MainWindow::doMove()
{
    if(!ui->move->isChecked())
        return;

    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
        if(i.value()->have_informations && i.value()->socket->isValid())
        {
            if(i.value()->direction==CatchChallenger::Direction_look_at_bottom)
            {
                i.value()->direction=CatchChallenger::Direction_look_at_left;
                i.value()->api->send_player_move(0,i.value()->direction);
            }
            else if(i.value()->direction==CatchChallenger::Direction_look_at_left)
            {
                i.value()->direction=CatchChallenger::Direction_look_at_top;
                i.value()->api->send_player_move(0,i.value()->direction);
            }
            else if(i.value()->direction==CatchChallenger::Direction_look_at_top)
            {
                i.value()->direction=CatchChallenger::Direction_look_at_right;
                i.value()->api->send_player_move(0,i.value()->direction);
            }
            else
            {
                i.value()->direction=CatchChallenger::Direction_look_at_bottom;
                i.value()->api->send_player_move(0,i.value()->direction);
            }
        }
    }
}

void MainWindow::doText()
{
    if(apiToCatchChallengerClient.isEmpty())
        return;
    if(!ui->randomText->isEnabled())
        return;
    if(!ui->randomText->isChecked())
        return;

    QList<CatchChallengerClient *> clientList;
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        clientList << i.value();
    }
    CatchChallengerClient *client=clientList.at(rand()%clientList.size());
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
    if(client->have_informations && ui->move->isChecked() && client->socket->isValid())
    {
        if(CommonSettings::commonSettings.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    client->api->sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    client->api->sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    client->api->sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettings::commonSettings.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

//quint32,QString,quint16,quint16,quint8,quint16
void MainWindow::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    ui->statusBar->showMessage(tr("On the map"));
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    if(player.simplifiedId==apiToCatchChallengerClient[senderObject]->api->getId())
        apiToCatchChallengerClient[senderObject]->direction=direction;
    apiToCatchChallengerClient[senderObject]->have_informations=true;
}

void MainWindow::haveCharacter()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;
    ui->statusBar->showMessage(QStringLiteral("Now on the map"));
}

void MainWindow::logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient[senderObject]->charactersList=characterEntryList;

    ui->characterList->clear();
    if(!ui->multipleConnexion->isChecked())
    {
        int index=0;
        while(index<characterEntryList.size())
        {
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(index);
            ui->characterList->addItem(character.pseudo,character.character_id);
            index++;
        }
    }
    ui->characterList->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
    ui->characterSelect->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
    if(apiToCatchChallengerClient.size()==1)
    {
        if(!CommonSettings::commonSettings.chat_allow_all && !CommonSettings::commonSettings.chat_allow_local)
        {
            ui->randomText->setEnabled(false);
            ui->randomText->setChecked(false);
            ui->chatRandomReply->setEnabled(false);
            ui->chatRandomReply->setChecked(false);
        }
        //get the datapack
        apiToCatchChallengerClient[senderObject]->api->sendDatapackContent();
        return;
    }
    if(apiToCatchChallengerClient[senderObject]->charactersList.count()<=0)
    {
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "have not character";
        if(ui->autoCreateCharacter->isChecked())
        {
            qDebug() << apiToCatchChallengerClient[senderObject]->login << "create new character";
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettings::commonSettings.max_pseudo_size-3);
            QString skin;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.isEmpty())
                skin=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skin=skinsList.at(rand()%skinsList.size()).fileName();
            apiToCatchChallengerClient[senderObject]->api->addCharacter(profileIndex,pseudo,skin);
        }
        return;
    }
    if(ui->multipleConnexion->isChecked())
    {
        const quint32 &character_id=apiToCatchChallengerClient[senderObject]->charactersList.at(rand()%apiToCatchChallengerClient[senderObject]->charactersList.size()).character_id;
        if(!characterOnMap.contains(character_id))
        {
            characterOnMap << character_id;
            qDebug() << "Select character:" << character_id;
            apiToCatchChallengerClient[senderObject]->api->selectCharacter(character_id);
        }
        return;
    }
}

void MainWindow::haveTheDatapack()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    //load the datapack
    {
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));
        //load the skins list
        QDir dir(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/skin/fighter/"));
        QFileInfoList entryList=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
        int index=0;
        while(index<entryList.size())
        {
            skinsList << entryList.at(index);
            index++;
        }
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.isEmpty())
    {
        qDebug() << "Profile list is empty";
        return;
    }

    if(apiToCatchChallengerClient[senderObject]->charactersList.count()<=0)
    {
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "have not character";
        if(ui->autoCreateCharacter->isChecked())
        {
            qDebug() << apiToCatchChallengerClient[senderObject]->login << "create new character";
            ui->characterSelect->setEnabled(false);
            ui->characterList->setEnabled(false);
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettings::commonSettings.max_pseudo_size-3);
            QString skin;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.isEmpty())
                skin=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skin=skinsList.at(rand()%skinsList.size()).fileName();
            apiToCatchChallengerClient[senderObject]->api->addCharacter(profileIndex,pseudo,skin);
        }
        return;
    }
    if(ui->multipleConnexion->isChecked())
    {
        connect(&connectTimer,&QTimer::timeout,this,&MainWindow::connectTimerSlot);
        connectTimer.start(1000/ui->connectBySeconds->value());

        //for the other client
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
        ui->groupBox_MultipleConnexion->setEnabled(false);

        //the actual client
        const quint32 &character_id=apiToCatchChallengerClient[senderObject]->charactersList.at(rand()%apiToCatchChallengerClient[senderObject]->charactersList.size()).character_id;
        if(!characterOnMap.contains(character_id))
        {
            characterOnMap << character_id;
            qDebug() << "Select character:" << character_id;
            apiToCatchChallengerClient[senderObject]->api->selectCharacter(character_id);
        }

        return;
    }
}

void MainWindow::connectTimerSlot()
{
    if(apiToCatchChallengerClient.size()<ui->connexionCount->value())
    {
        const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
        if(diff<=(quint32)ui->maxDiffConnectedSelected->value())
            createClient()->socket->connectToHost(ui->host->text(),ui->port->value());
    }
    else
        connectTimer.stop();
}

void MainWindow::newCharacterId(const quint32 &characterId)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    if(!characterOnMap.contains(characterId))
    {
        qDebug() << "Select new character created:" << characterId;
        characterOnMap << characterId;
        apiToCatchChallengerClient[senderObject]->api->selectCharacter(characterId);
    }
}

void MainWindow::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;
    apiToCatchChallengerClient[senderObject]->selectedCharacter=true;
    numberOfSelectedCharacter++;
    ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));

    Q_UNUSED(informations);
//    DebugClass::debugConsole(QStringLiteral("MainWindow::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void MainWindow::newError(QString error,QString detailedError)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    ui->statusBar->showMessage(QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError));
    CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() error: %1, detailedError: %2").arg(error).arg(detailedError));
    apiToCatchChallengerClient[senderObject]->socket->disconnectFromHost();
}

void MainWindow::newSocketError(QAbstractSocket::SocketError error)
{
    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
        return;

    connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    if(error==0)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() Connection refused").arg(error));
        ui->statusBar->showMessage(QStringLiteral("Connection refused").arg(error));
    }
    else if(error==13)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() SslHandshakeFailedError"));
        ui->statusBar->showMessage(QStringLiteral("SslHandshakeFailedError"));
    }
    else
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() error: %1").arg(error));
        ui->statusBar->showMessage(QStringLiteral("Error: %1").arg(error));
    }

}

void MainWindow::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    if(!ui->chatRandomReply->isChecked() && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    Q_UNUSED(text);
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    Q_UNUSED(type);
    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettings::commonSettings.chat_allow_all)
            switch(rand()%(100*apiToCatchChallengerClient.size()))
            {
                case 0:
                    apiToCatchChallengerClient[senderObject]->api->sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettings::commonSettings.chat_allow_local)
            switch(rand()%(3*apiToCatchChallengerClient.size()))
            {
                case 0:
                    apiToCatchChallengerClient[senderObject]->api->sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettings::commonSettings.chat_allow_private)
            apiToCatchChallengerClient[senderObject]->api->sendPM(QStringLiteral("Hello %1, I'm few bit busy for now").arg(pseudo),pseudo);
        break;
        default:
        break;
    }
}

void MainWindow::on_connect_clicked()
{
    if(!ui->connect->isEnabled())
        return;
    if(ui->pass->text().size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    if(ui->login->text().size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }
    ui->groupBox_MultipleConnexion->setEnabled(false);
    ui->groupBox_Proxy->setEnabled(false);
    settings.setValue("login",ui->login->text());
    settings.setValue("pass",ui->pass->text());
    settings.setValue("host",ui->host->text());
    settings.setValue("port",ui->port->value());
    settings.setValue("proxy",ui->proxy->text());
    settings.setValue("proxyport",ui->proxyport->value());
    if(ui->multipleConnexion->isChecked())
        settings.setValue("multipleConnexion",ui->connexionCount->value());
    else
        settings.setValue("multipleConnexion",0);
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());

    if(!ui->connect->isEnabled())
        return;
    ui->connect->setEnabled(false);


    //do only the first client to download the datapack
    createClient()->socket->connectToHost(ui->host->text(),ui->port->value());
}

MainWindow::CatchChallengerClient * MainWindow::createClient()
{
    QNetworkProxy proxy;
    if(!ui->proxy->text().isEmpty())
    {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(ui->proxy->text());
        proxy.setPort(ui->proxyport->value());
    }

    CatchChallengerClient * client=new CatchChallengerClient;
    client->sslSocket=new QSslSocket();
    client->socket=new CatchChallenger::ConnectedSocket(client->sslSocket);
    client->api=new CatchChallenger::Api_client_real(client->socket,false);
    client->sslSocket->ignoreSslErrors();
    client->sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    client->api->setDatapackPath(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));
    connect(client->sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    connect(client->api,&CatchChallenger::Api_client_real::insert_player,            this,&MainWindow::insert_player);
    connect(client->api,&CatchChallenger::Api_client_real::new_chat_text,            this,&MainWindow::new_chat_text,Qt::QueuedConnection);
    connect(client->api,&CatchChallenger::Api_client_real::haveCharacter,            this,&MainWindow::haveCharacter);
    connect(client->api,&CatchChallenger::Api_client_real::logged,                   this,&MainWindow::logged);
    connect(client->api,&CatchChallenger::Api_client_real::have_current_player_info, this,&MainWindow::have_current_player_info);
    connect(client->api,&CatchChallenger::Api_client_real::newError,                 this,&MainWindow::newError);
    connect(client->api,&CatchChallenger::Api_client_real::newCharacterId,           this,&MainWindow::newCharacterId);
    connect(client->socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),                    this,&MainWindow::newSocketError);
    connect(client->socket,&CatchChallenger::ConnectedSocket::disconnected,          this,&MainWindow::disconnected);
    connect(client->socket,&CatchChallenger::ConnectedSocket::connected,             this,&MainWindow::tryLink,Qt::QueuedConnection);
    if(apiToCatchChallengerClient.isEmpty())
        connect(client->api,&CatchChallenger::Api_client_real::haveTheDatapack,      this,&MainWindow::haveTheDatapack);
    client->sslSocket->setProxy(proxy);
    client->haveShowDisconnectionReason=false;
    client->have_informations=false;
    client->number=number;
    client->selectedCharacter=false;
    number++;
    apiToCatchChallengerClient[client->api]=client;
    connectedSocketToCatchChallengerClient[client->socket]=client;
    sslSocketToCatchChallengerClient[client->sslSocket]=client;
    return client;
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    QSslSocket *senderObject = qobject_cast<QSslSocket *>(sender());
    if(senderObject==NULL)
        return;

    QStringList sslErrors;
    int index=0;
    while(index<errors.size())
    {
        qDebug() << "Ssl error:" << errors.at(index).errorString();
        sslErrors << errors.at(index).errorString();
        index++;
    }
    /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
    realSocket->disconnectFromHost();*/
}

void MainWindow::on_characterSelect_clicked()
{
    if(ui->characterList->count()<=0 || !ui->characterSelect->isEnabled())
        return;
    const quint32 &charId=ui->characterList->currentData().toUInt();
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        qDebug() << "Select character:" << charId;
        i.value()->api->selectCharacter(charId);
        characterOnMap << charId;
    }
    ui->characterSelect->setEnabled(false);
    ui->characterList->setEnabled(false);
}