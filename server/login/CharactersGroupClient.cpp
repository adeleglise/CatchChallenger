#include "CharactersGroupForLogin.h"
#include "EpollServerLoginSlave.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/SqlFunction.h"
#include "../base/PreparedDBQuery.h"
#include "../base/DictionaryLogin.h"
#include "../../general/base/CommonSettingsCommon.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

void CharactersGroupForLogin::character_list(EpollClientLoginSlave * const client,const quint32 &account_id)
{
    const QString &queryText=QString(PreparedDBQueryCommon::db_query_characters).arg(account_id).arg(CommonSettingsCommon::commonSettingsCommon.max_character*2);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::character_list_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        client->askLogin_cancel();
        return;
    }
    else
        clientQueryForReadReturn << client;
}

void CharactersGroupForLogin::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->character_list_object();
}

void CharactersGroupForLogin::character_list_object()
{
    EpollClientLoginSlave * const client=clientQueryForReadReturn.takeFirst();

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));//performance
    int tempRawDataSize=0x01;

    const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
    bool ok;
    quint8 validCharaterCount=0;
    while(databaseBaseCommon->next() && validCharaterCount<CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        unsigned int character_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            //delete
            quint32 time_to_delete=QString(databaseBaseCommon->value(3)).toUInt(&ok);
            if(!ok)
            {
                qDebug() << (QStringLiteral("time_to_delete is not number: %1 for %2 fixed by 0").arg(QString(databaseBaseCommon->value(3))).arg(character_id));
                time_to_delete=0;
            }
            if(time_to_delete==0 || current_time<time_to_delete)
            {
                //send the char, not delete

                validCharaterCount++;

                //Character id
                {
                    *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(character_id);
                    tempRawDataSize+=sizeof(quint32);
                }

                //can't be empty or have wrong or too large utf8 data
                //pseudo
                {
                    const quint8 &newSize=FacilityLibGeneral::toUTF8WithHeader(databaseBaseCommon->value(1),tempRawData+tempRawDataSize);
                    if(newSize==0)
                    {
                        qDebug() << (QStringLiteral("can't be empty or have wrong or too large utf8 data: %1 by hide this char").arg(databaseBaseCommon->value(1)));
                        tempRawDataSize-=sizeof(quint32);
                        validCharaterCount--;
                        continue;
                    }
                    tempRawDataSize+=newSize;
                }

                //skin
                {
                    const quint32 databaseSkinId=QString(databaseBaseCommon->value(2)).toUInt(&ok);
                    if(!ok)//if not number
                    {
                        qDebug() << (QStringLiteral("character return skin is not number: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(5)).arg(character_id));
                        tempRawData[tempRawDataSize]=0;
                        ok=true;
                    }
                    else
                    {
                        if(databaseSkinId>=(quint32)DictionaryLogin::dictionary_skin_database_to_internal.size())//out of range
                        {
                            qDebug() << (QStringLiteral("character return skin out of range: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(5)).arg(character_id));
                            tempRawData[tempRawDataSize]=0;
                            ok=true;
                        }
                        //else all is good
                        else
                            tempRawData[tempRawDataSize]=DictionaryLogin::dictionary_skin_database_to_internal.at(databaseSkinId);
                    }
                    tempRawDataSize+=1;
                }

                //delete register
                {
                    unsigned int delete_time_left=0;
                    if(time_to_delete==0)
                        delete_time_left=0;
                    else
                        delete_time_left=time_to_delete-current_time;
                    *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(delete_time_left);
                    tempRawDataSize+=sizeof(quint32);
                }

                //played_time
                {
                    unsigned int played_time=QString(databaseBaseCommon->value(4)).toUInt(&ok);
                    if(!ok)
                    {
                        qDebug() << (QStringLiteral("played_time is not number: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(4)).arg(character_id));
                        played_time=0;
                    }
                    *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(played_time);
                    tempRawDataSize+=sizeof(quint32);
                }

                //last_connect
                {
                    unsigned int last_connect=QString(databaseBaseCommon->value(5)).toUInt(&ok);
                    if(!ok)
                    {
                        qDebug() << (QStringLiteral("last_connect is not number: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(5)).arg(character_id));
                        last_connect=current_time;
                    }
                    *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                    tempRawDataSize+=sizeof(quint32);
                }
            }
            else
                deleteCharacterNow(character_id);
        }
        else
            qDebug() << (QStringLiteral("Server id is not number: %1 for %2").arg(databaseBaseCommon->value(0)).arg(character_id));
    }
    tempRawData[0]=validCharaterCount;

    client->character_list_return(this->index,tempRawData,tempRawDataSize);
    //delete tempRawData;//delete later to order the list, see EpollClientLoginSlave::server_list_return(), QMapIterator<quint8,CharacterListForReply> i(characterTempListForReply);, delete i.value().rawData;

    //get server list
    server_list(client,client->account_id);
}

void CharactersGroupForLogin::server_list(EpollClientLoginSlave * const client,const quint32 &account_id)
{
    const QString &queryText=QString(PreparedDBQueryCommon::db_query_select_server_time).arg(account_id);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::server_list_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        client->askLogin_cancel();
        return;
    }
    else
        clientQueryForReadReturn << client;
}

void CharactersGroupForLogin::server_list_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->server_list_object();
}

void CharactersGroupForLogin::server_list_object()
{
    EpollClientLoginSlave * const client=clientQueryForReadReturn.takeFirst();

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x00;

    const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
    bool ok;
    quint8 validServerCount=0;
    while(databaseBaseCommon->next() && validServerCount<CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        const unsigned int server_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            //global over the server group
            if(servers.contains(server_id))
            {
                //server index
                tempRawData[tempRawDataSize]=servers.value(server_id).indexOnFlatList;
                tempRawDataSize+=1;

                //played_time
                unsigned int played_time=QString(databaseBaseCommon->value(1)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("played_time is not number: %1 fixed by 0").arg(databaseBaseCommon->value(4)));
                    played_time=0;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(played_time);
                tempRawDataSize+=sizeof(quint32);

                //last_connect
                unsigned int last_connect=QString(databaseBaseCommon->value(2)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("last_connect is not number: %1 fixed by 0").arg(databaseBaseCommon->value(5)));
                    last_connect=current_time;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                tempRawDataSize+=sizeof(quint32);

                validServerCount++;
            }
        }
        else
            qDebug() << (QStringLiteral("Character id is not number: %1").arg(databaseBaseCommon->value(0)));
    }

    client->server_list_return(validServerCount,tempRawData,tempRawDataSize);
    delete tempRawData;
}

void CharactersGroupForLogin::deleteCharacterNow(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_monster_by_character_id==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_buff==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_skill==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_character==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_character is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_all_item==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_item is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_by_character==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_monster is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_recipes==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_reputation==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_reputation is empty, bug"));
        return;
    }
    #endif

    const QString &queryText=QString(PreparedDBQueryCommon::db_query_monster_by_character_id).arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::deleteCharacterNow_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        return;
    }
    else
        deleteCharacterNowCharacterIdList << characterId;
}

void CharactersGroupForLogin::deleteCharacterNow_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->deleteCharacterNow_object();
}

void CharactersGroupForLogin::deleteCharacterNow_object()
{
    deleteCharacterNow_return(deleteCharacterNowCharacterIdList.takeFirst());
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::deleteCharacterNow_return(const quint32 &characterId)
{
    bool ok;
    while(databaseBaseCommon->next())
    {
        const quint32 &monsterId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_monster_buff).arg(monsterId).toUtf8().constData());
            dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_monster_skill).arg(monsterId).toUtf8().constData());
        }
    }
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_character).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_all_item).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_all_item_warehouse).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_monster_by_character).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_recipes).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_reputation).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQueryCommon::db_query_delete_allow).arg(characterId).toUtf8().constData());
}

qint8 CharactersGroupForLogin::addCharacter(void * const client,const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_character_by_pseudo==NULL)
    {
        qDebug() << (QStringLiteral("addCharacter() Query is empty, bug"));
        return 0x03;
    }
    #endif
    if(DictionaryLogin::dictionary_skin_internal_to_database.isEmpty())
    {
        qDebug() << QStringLiteral("Skin list is empty, unable to add charaters");
        //char tempData[1+4]={0x02,0x00,0x00,0x00,0x00};/* not htole32 because inverted is the same */
        //static_cast<EpollClientLoginSlave *>(client)->postReply(query_id,tempData,sizeof(tempData));
        return 0x03;
    }
    /** \warning Need be checked in real time because can be opened on multiple login server
     * if(static_cast<EpollClientLoginSlave *>(client)->accountCharatersCount>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        qDebug() << (QStringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(static_cast<EpollClientLoginSlave *>(client)->accountCharatersCount).arg(CommonSettingsCommon::commonSettingsCommon.max_character));
        return -1;
    }*/
    if(profileIndex>=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.size())
    {
        qDebug() << (QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.size()));
        return -1;
    }
    if(pseudo.isEmpty())
    {
        qDebug() << (QStringLiteral("pseudo is empty, not allowed"));
        return -1;
    }
    if((quint32)pseudo.size()>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
    {
        qDebug() << (QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
        return -1;
    }
    if(skinId>=DictionaryLogin::dictionary_skin_internal_to_database.size())
    {
        qDebug() << (QStringLiteral("skin provided: %1 is not into skin listed").arg(skinId));
        return -1;
    }
    const EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.at(profileIndex);
    if(std::find(profile.forcedskin.begin(),profile.forcedskin.end(),skinId)==profile.forcedskin.end())
    {
        qDebug() << (QStringLiteral("skin provided: %1 is not into profile %2 forced skin list").arg(skinId).arg(profileIndex));
        return -1;
    }
    AddCharacterParam addCharacterParam;
    addCharacterParam.query_id=query_id;
    addCharacterParam.profileIndex=profileIndex;
    addCharacterParam.pseudo=pseudo;
    addCharacterParam.skinId=skinId;
    addCharacterParam.client=client;

    const QString &queryText=QString(PreparedDBQueryCommon::db_query_get_character_count_by_account).arg(static_cast<EpollClientLoginSlave *>(client)->account_id);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::addCharacterStep1_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        return 0x03;
    }
    else
    {
        addCharacterParamList << addCharacterParam;
        return 0x00;
    }
}

void CharactersGroupForLogin::addCharacterStep1_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->addCharacterStep1_object();
}

void CharactersGroupForLogin::addCharacterStep1_object()
{
    AddCharacterParam addCharacterParam=addCharacterParamList.takeFirst();
    addCharacterStep1_return(static_cast<EpollClientLoginSlave * const>(addCharacterParam.client),addCharacterParam.query_id,addCharacterParam.profileIndex,addCharacterParam.pseudo,addCharacterParam.skinId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacterStep1_return(EpollClientLoginSlave * const client,const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId)
{
    if(!databaseBaseCommon->next())
    {
        qDebug() << QStringLiteral("Character count query return nothing");
        client->addCharacter_ReturnFailed(query_id,0x03);
        return;
    }
    bool ok;
    quint32 characterCount=QString(databaseBaseCommon->value(0)).toUInt(&ok);
    if(!ok)
    {
        qDebug() << QStringLiteral("Character count query return not a number");
        client->addCharacter_ReturnFailed(query_id,0x03);
        return;
    }
    if(characterCount>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        qDebug() << (QStringLiteral("You can't create more account, you have already %1 on %2 allowed")
                     .arg(characterCount)
                     .arg(CommonSettingsCommon::commonSettingsCommon.max_character)
                     );
        client->addCharacter_ReturnFailed(query_id,0x02);
        return;
    }

    const QString &queryText=QString(PreparedDBQueryCommon::db_query_select_character_by_pseudo).arg(SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::addCharacterStep2_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        return;
    }
    else
    {
        AddCharacterParam addCharacterParam;
        addCharacterParam.query_id=query_id;
        addCharacterParam.profileIndex=profileIndex;
        addCharacterParam.pseudo=pseudo;
        addCharacterParam.skinId=skinId;
        addCharacterParam.client=client;
        addCharacterParamList << addCharacterParam;
        return;
    }
}

void CharactersGroupForLogin::addCharacterStep2_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->addCharacterStep2_object();
}

void CharactersGroupForLogin::addCharacterStep2_object()
{
    AddCharacterParam addCharacterParam=addCharacterParamList.takeFirst();
    addCharacterStep2_return(static_cast<EpollClientLoginSlave * const>(addCharacterParam.client),addCharacterParam.query_id,addCharacterParam.profileIndex,addCharacterParam.pseudo,addCharacterParam.skinId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacterStep2_return(EpollClientLoginSlave * const client,const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId)
{
    if(databaseBaseCommon->next())
    {
        client->addCharacter_ReturnFailed(query_id,0x01);
        return;
    }
    const EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.at(profileIndex);

    const quint32 &characterId=maxCharacterId.back();
    maxCharacterId.pop_back();
    unsigned int index=0;
    int monster_position=1;
    int tempBufferSize=0;
    QByteArray numberBuffer;

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[0],profile.preparedQuerySize[0]);
    tempBufferSize+=profile.preparedQuerySize[0];

    numberBuffer=QString::number(characterId).toLatin1();
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.constData(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[1],profile.preparedQuerySize[1]);
    tempBufferSize+=profile.preparedQuerySize[1];

    numberBuffer=QString::number(client->account_id).toLatin1();
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.constData(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[2],profile.preparedQuerySize[2]);
    tempBufferSize+=profile.preparedQuerySize[2];

    numberBuffer=SqlFunction::quoteSqlVariable(pseudo).toUtf8();
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.constData(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[3],profile.preparedQuerySize[3]);
    tempBufferSize+=profile.preparedQuerySize[3];

    numberBuffer=QString::number(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId)).toLatin1();
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.constData(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[4],profile.preparedQuerySize[4]);
    tempBufferSize+=profile.preparedQuerySize[4];

    numberBuffer=QString::number(QDateTime::currentMSecsSinceEpoch()/1000).toLatin1();
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.constData(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[5],profile.preparedQuerySize[5]);
    tempBufferSize+=profile.preparedQuerySize[5];

    CharactersGroupForLogin::tempBuffer[tempBufferSize]='\0';

    dbQueryWriteCommon(CharactersGroupForLogin::tempBuffer);
    while(index<profile.monsters.size())
    {
        const EpollServerLoginSlave::LoginProfile::Monster &monster=profile.monsters.at(index);
        quint32 gender=Gender_Unknown;
        if(monster.ratio_gender!=-1)
        {
            if(rand()%101<monster.ratio_gender)
                gender=Gender_Female;
            else
                gender=Gender_Male;
        }

        quint32 monster_id=maxMonsterId.back();
        maxMonsterId.pop_back();

        //insert the monster is db
        {
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster
               .arg(monster_id)
               .arg(monster.hp)
               .arg(characterId)
               .arg(monster.id)
               .arg(monster.level)
               .arg(monster.captured_with)
               .arg(gender)
               .arg(monster_position)
               .toUtf8().constData());
            monster_position++;
        }

        //insert the skill
        unsigned int sub_index=0;
        while(sub_index<monster.skills.size())
        {
            const EpollServerLoginSlave::LoginProfile::Monster::Skill &skill=monster.skills.at(sub_index);
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_skill
               .arg(monster_id)
               .arg(skill.id)
               .arg(skill.level)
               .arg(skill.endurance)
               .toUtf8().constData());
            sub_index++;
        }
        index++;
    }
    index=0;
    while(index<profile.reputation.size())
    {
        const EpollServerLoginSlave::LoginProfile::Reputation &reputation=profile.reputation.at(index);
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_reputation
           .arg(characterId)
           .arg(reputation.reputationDatabaseId)
           .arg(reputation.point)
           .arg(reputation.level)
           .toUtf8().constData());
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_item
           .arg(profile.items.at(index).id)
           .arg(characterId)
           .arg(profile.items.at(index).quantity)
           .toUtf8().constData());
        index++;
    }

    //send the network reply
    CharactersGroupForLogin::tempBuffer[0]=0x00;
    *reinterpret_cast<quint32 *>(CharactersGroupForLogin::tempBuffer+1)=htole32(characterId);
    client->postReply(query_id,CharactersGroupForLogin::tempBuffer,1+4);
}

bool CharactersGroupForLogin::removeCharacter(void * const client,const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id==NULL)
    {
        qDebug() << (QStringLiteral("removeCharacter() Query is empty, bug"));
        return false;
    }
    #endif
    RemoveCharacterParam removeCharacterParam;
    removeCharacterParam.query_id=query_id;
    removeCharacterParam.characterId=characterId;
    removeCharacterParam.client=client;

    const QString &queryText=PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::removeCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        return false;
    }
    else
    {
        removeCharacterParamList << removeCharacterParam;
        return true;
    }
}

void CharactersGroupForLogin::removeCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->removeCharacter_object();
}

void CharactersGroupForLogin::removeCharacter_object()
{
    RemoveCharacterParam removeCharacterParam=removeCharacterParamList.takeFirst();
    removeCharacter_return(static_cast<EpollClientLoginSlave *>(removeCharacterParam.client),removeCharacterParam.query_id,removeCharacterParam.characterId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::removeCharacter_return(EpollClientLoginSlave * const client,const quint8 &query_id,const quint32 &characterId)
{
    if(!databaseBaseCommon->next())
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,QStringLiteral("Result return query to remove wrong"));
        return;
    }
    bool ok;
    const quint32 &account_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
    if(!ok)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(databaseBaseCommon->value(0)));
        return;
    }
    if(client->account_id!=account_id)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(account_id));
        return;
    }
    const quint32 &time_to_delete=QString(databaseBaseCommon->value(1)).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,QStringLiteral("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(account_id));
        return;
    }
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id.arg(characterId).arg(
                           //date to delete, not time (no sens)
                           QDateTime::currentDateTime().toTime_t()+
                           CommonSettingsCommon::commonSettingsCommon.character_delete_time
                           ).toUtf8().constData());
    CharactersGroupForLogin::tempBuffer[0]=0x02;
    client->postReply(query_id,CharactersGroupForLogin::tempBuffer,1);
}

void CharactersGroupForLogin::dbQueryWriteCommon(const char * const queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText==NULL || queryText[0]=='\0')
    {
        qDebug() << (QStringLiteral("dbQuery() Query is empty, bug"));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    qDebug() << (QStringLiteral("Do common db write: ")+queryText);
    #endif
    databaseBaseCommon->asyncWrite(queryText);
}