#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"

using namespace CatchChallenger;

CommonFightEngine::CommonFightEngine()
{
    resetAll();
}

bool CommonFightEngine::tryEscape()
{
    if(!isInFight())//check if is in fight
    {
        emit error(QString("error: tryEscape() when is not in fight"));
        return false;
    }
    if(wildMonsters.isEmpty())
    {
        emit message("You can't escape because it's not a wild monster");
        return false;
    }
    return true;
}

void CommonFightEngine::resetAll()
{
    stepFight_Grass=0;
    stepFight_Water=0;
    stepFight_Cave=0;
    player_informations=NULL;
    ableToFight=false;
    wildMonsters.clear();
    botFightMonsters.clear();
    randomSeeds.clear();
    selectedMonster=0;
}

bool CommonFightEngine::isInFight()
{
    return !wildMonsters.empty() || !botFightMonsters.isEmpty();
}

void CommonFightEngine::newRandomNumber(const QByteArray &randomData)
{
    randomSeeds+=randomData;
}

void CommonFightEngine::setVariable(Player_private_and_public_informations *player_informations)
{
    this->player_informations=player_informations;
}

bool CommonFightEngine::getAbleToFight()
{
    return ableToFight;
}

//return is have random seed to do random step
bool CommonFightEngine::canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y)
{
    if(isInFight())
    {
        emit message(QString("map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y));
        return false;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(map,x,y) && !map.grassMonster.empty())
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;
    if(CatchChallenger::MoveOnTheMap::isWater(map,x,y) && !map.waterMonster.empty())
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;
    if(!map.caveMonster.empty())
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    emit message(QString("map: %1 (%2,%3), no fight in this zone").arg(map.map_file).arg(x).arg(y));
    return true;
}

PlayerMonster CommonFightEngine::getRandomMonster(const QList<MapMonster> &monsterList,bool *ok)
{
    PlayerMonster playerMonster;
    playerMonster.captured_with=0;
    playerMonster.egg_step=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    quint8 randomMonsterInt=getOneSeed(100);
    bool monsterFound=false;
    int index=0;
    while(index<monsterList.size())
    {
        int luck=monsterList.at(index).luck;
        if(randomMonsterInt<luck)
        {
            //it's this monster
            playerMonster.monster=monsterList.at(index).id;
            //select the level
            if(monsterList.at(index).maxLevel==monsterList.at(index).minLevel)
                playerMonster.level=monsterList.at(index).minLevel;
            else
                playerMonster.level=getOneSeed(monsterList.at(index).maxLevel-monsterList.at(index).minLevel+1)+monsterList.at(index).minLevel;
            monsterFound=true;
            break;
        }
        else
            randomMonsterInt-=luck;
        index++;
    }
    if(!monsterFound)
    {
        emit error(QString("error: no wild monster selected"));
        *ok=false;
        playerMonster.monster=0;
        playerMonster.level=0;
        playerMonster.gender=Gender_Unknown;
        return playerMonster;
    }
    Monster monsterDef=CommonDatapack::commonDatapack.monsters[playerMonster.monster];
    if(monsterDef.ratio_gender>0 && monsterDef.ratio_gender<100)
    {
        qint8 temp_ratio=getOneSeed(101);
        if(temp_ratio<monsterDef.ratio_gender)
            playerMonster.gender=Gender_Male;
        else
            playerMonster.gender=Gender_Female;
    }
    else
    {
        switch(monsterDef.ratio_gender)
        {
            case 0:
                playerMonster.gender=Gender_Male;
            break;
            case 100:
                playerMonster.gender=Gender_Female;
            break;
            default:
                playerMonster.gender=Gender_Unknown;
            break;
        }
    }
    Monster::Stat monsterStat=getStat(monsterDef,playerMonster.level);
    playerMonster.hp=monsterStat.hp;
    index=monsterDef.learn.size()-1;
    while(index>=0 && playerMonster.skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER)
    {
        if(monsterDef.learn.at(index).learnAtLevel<=playerMonster.level)
        {
            PlayerMonster::PlayerSkill temp;
            temp.level=monsterDef.learn.at(index).learnSkillLevel;
            temp.skill=monsterDef.learn.at(index).learnSkill;
            playerMonster.skills << temp;
        }
        index--;
    }
    *ok=true;
    return playerMonster;
}

/** \warning you need check before the input data */
Monster::Stat CommonFightEngine::getStat(const Monster &monster, const quint8 &level)
{
    Monster::Stat stat=monster.stat;
    stat.attack=stat.attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.attack==0)
        stat.attack=1;
    stat.defense=stat.defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.defense==0)
        stat.defense=1;
    stat.hp=stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.hp==0)
        stat.hp=1;
    stat.special_attack=stat.special_attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.special_attack==0)
        stat.special_attack=1;
    stat.special_defense=stat.special_defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.special_defense==0)
        stat.special_defense=1;
    stat.speed=stat.speed*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.speed==0)
        stat.speed=1;
    return stat;
}

void CommonFightEngine::updateCanDoFight()
{
    ableToFight=false;
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        return;
    }
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->playerMonster.at(index);
        if(!monsterIsKO(playerMonsterEntry))
        {
            selectedMonster=index;
            ableToFight=true;
            return;
        }
        index++;
    }
}

PlayerMonster * CommonFightEngine::getCurrentMonster()
{
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        return NULL;
    }
    if(selectedMonster>0 && selectedMonster<player_informations->playerMonster.size())
        return &player_informations->playerMonster[selectedMonster];
    else
    {
        emit error(QString("selectedMonster is out of range"));
        return NULL;
    }
}

quint8 CommonFightEngine::getCurrentSelectedMonsterNumber()
{
    return selectedMonster;
}

quint8 CommonFightEngine::getOtherSelectedMonsterNumber()
{
    return 0;
}

PublicPlayerMonster * CommonFightEngine::getOtherMonster()
{
    PublicPlayerMonster *publicPlayerMonster;
    if(!wildMonsters.isEmpty())
        publicPlayerMonster=&wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        publicPlayerMonster=&botFightMonsters.first();
    return publicPlayerMonster;
}

bool CommonFightEngine::remainMonstersToFight(const quint32 &monsterId) const
{
    if(player_informations==NULL)
    {
        qDebug() << "player_informations is NULL";
        return false;
    }
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->playerMonster.at(index);
        if(playerMonsterEntry.id==monsterId)
        {
            //the current monster can't fight, echange it will do nothing
            if(monsterIsKO(playerMonsterEntry))
                return true;
        }
        else
        {
            //other monster can fight, can continue to fight
            if(!monsterIsKO(playerMonsterEntry))
                return true;
        }
        index++;
    }
    return false;
}

bool CommonFightEngine::monsterIsKO(const PlayerMonster &playerMonter)
{
    return playerMonter.hp<=0 || playerMonter.egg_step>0;
}

Skill::AttackReturn CommonFightEngine::generateOtherAttack()
{
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=false;
    attackReturn.success=false;
    PlayerMonster otherMonster;
    if(!wildMonsters.isEmpty())
        otherMonster=wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        otherMonster=botFightMonsters.first();
    else
    {
        emit error("no other monster found");
        return attackReturn;
    }
    if(otherMonster.skills.empty())
        return attackReturn;
    int position;
    if(otherMonster.skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster.skills.size());
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster.skills.at(position);
    attackReturn.attack=otherMonsterSkill.skill;
    const Skill::SkillList &skillList=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[otherMonsterSkill.skill].level.at(otherMonsterSkill.level-1);
    int index=0;
    while(index<skillList.buff.size())
    {
        const Skill::Buff &buff=skillList.buff.at(index);
        bool success;
        if(buff.success==100)
            success=true;
        else
            success=(getOneSeed(100)<buff.success);
        if(success)
        {
            applyOtherBuffEffect(buff.effect);
            attackReturn.buffEffectMonster << buff.effect;
            attackReturn.success=true;
        }
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        const Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
        {
            attackReturn.lifeEffectMonster << applyOtherLifeEffect(life.effect);
            attackReturn.success=true;
        }
        index++;
    }
    return attackReturn;
}

Skill::LifeEffectReturn CommonFightEngine::applyOtherLifeEffect(const Skill::LifeEffect &effect)
{
    PlayerMonster *otherMonster;
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        Skill::LifeEffectReturn effect_to_return;
        effect_to_return.on=effect.on;
        effect_to_return.quantity=0;
        return effect_to_return;
    }
    if(!wildMonsters.isEmpty())
        otherMonster=&wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        otherMonster=&botFightMonsters.first();
    else
    {
        emit error(QString("Unable to locate the other monster to generate other attack"));
        Skill::LifeEffectReturn effect_to_return;
        effect_to_return.on=effect.on;
        effect_to_return.quantity=0;
        return effect_to_return;
    }
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster],otherMonster->level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->playerMonster[selectedMonster].monster],player_informations->playerMonster[selectedMonster].level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*otherMonster->level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(player_informations->playerMonster[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>(qint32)player_informations->playerMonster[selectedMonster].hp)
            {
                player_informations->playerMonster[selectedMonster].hp=0;
                player_informations->playerMonster[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(qint32)(stat.hp-player_informations->playerMonster[selectedMonster].hp))
                player_informations->playerMonster[selectedMonster].hp=stat.hp;
            else
                player_informations->playerMonster[selectedMonster].hp+=quantity;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*otherMonster->level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(otherMonster->hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>(qint32)otherMonster->hp)
                otherMonster->hp=0;
            else if(quantity>0 && quantity>(qint32)(stat.hp-otherMonster->hp))
                otherMonster->hp=stat.hp;
            else
                otherMonster->hp+=quantity;
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
    Skill::LifeEffectReturn effect_to_return;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void CommonFightEngine::applyOtherBuffEffect(const Skill::BuffEffect &effect)
{
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        return;
    }
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            player_informations->playerMonster[selectedMonster].buffs << tempBuff;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(wildMonsters.isEmpty())
                wildMonsters.first().buffs << tempBuff;
            else if(botFightMonsters.isEmpty())
                botFightMonsters.first().buffs << tempBuff;
            else
            {
                emit error(QString("Unable to locate the other monster to apply other buff effect"));
                return;
            }
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
}

Skill::LifeEffectReturn CommonFightEngine::applyCurrentLifeEffect(const Skill::LifeEffect &effect)
{
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        Skill::LifeEffectReturn effect_to_return;
        effect_to_return.on=effect.on;
        effect_to_return.quantity=0;
        return effect_to_return;
    }
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->playerMonster.at(selectedMonster).monster],player_informations->playerMonster.at(selectedMonster).level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {

            PublicPlayerMonster *publicPlayerMonster=getOtherMonster();
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[publicPlayerMonster->monster],publicPlayerMonster->level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*player_informations->playerMonster.at(selectedMonster).level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*player_informations->playerMonster.at(selectedMonster).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(publicPlayerMonster->hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>(qint32)publicPlayerMonster->hp)
                publicPlayerMonster->hp=0;
            else if(quantity>0 && quantity>(qint32)(stat.hp-publicPlayerMonster->hp))
                publicPlayerMonster->hp=stat.hp;
            else
                publicPlayerMonster->hp+=quantity;
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*player_informations->playerMonster.at(selectedMonster).level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*player_informations->playerMonster.at(selectedMonster).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(player_informations->playerMonster[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>(qint32)player_informations->playerMonster[selectedMonster].hp)
            {
                player_informations->playerMonster[selectedMonster].hp=0;
                player_informations->playerMonster[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(qint32)(stat.hp-player_informations->playerMonster[selectedMonster].hp))
                player_informations->playerMonster[selectedMonster].hp=stat.hp;
            else
                player_informations->playerMonster[selectedMonster].hp+=quantity;
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
    Skill::LifeEffectReturn effect_to_return;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void CommonFightEngine::applyCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            getOtherMonster()->buffs << tempBuff;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            getCurrentMonster()->buffs << tempBuff;
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
}

ApplyOn CommonFightEngine::invertApplyOn(const ApplyOn &applyOn)
{
    switch(applyOn)
    {
        case ApplyOn_AloneEnemy:
            return ApplyOn_Themself;
        case ApplyOn_AllEnemy:
            return ApplyOn_AllAlly;
        case ApplyOn_Themself:
            return ApplyOn_AloneEnemy;
        case ApplyOn_AllAlly:
            return ApplyOn_AllEnemy;
        default:
            return ApplyOn_Themself;
        break;
    }
}

bool CommonFightEngine::haveRandomFight(const Map &map,const quint8 &x,const quint8 &y)
{
    bool ok;
    if(isInFight())
    {
        emit error(QString("error: map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y));
        return false;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(map,x,y) && !map.grassMonster.empty())
    {
        if(stepFight_Grass<=0)
            stepFight_Grass=getOneSeed(16);
        else
            stepFight_Grass--;
        if(stepFight_Grass<=0)
        {
            PlayerMonster monster=getRandomMonster(map.grassMonster,&ok);
            if(ok)
                wildMonsters << monster;
            return ok;
        }
        else
            return false;
    }
    if(CatchChallenger::MoveOnTheMap::isWater(map,x,y) && !map.waterMonster.empty())
    {
        if(stepFight_Water<=0)
            stepFight_Water=getOneSeed(16);
        else
            stepFight_Water--;
        if(stepFight_Water<=0)
        {
            PlayerMonster monster=getRandomMonster(map.waterMonster,&ok);
            if(ok)
                wildMonsters << monster;
            return ok;
        }
        else
            return false;
    }
    if(!map.caveMonster.empty())
    {
        if(stepFight_Cave<=0)
            stepFight_Cave=getOneSeed(16);
        else
            stepFight_Cave--;
        if(stepFight_Cave<=0)
        {
            PlayerMonster monster=getRandomMonster(map.caveMonster,&ok);
            if(ok)
                wildMonsters << monster;
            return ok;
        }
        else
            return false;
    }

    /// no fight in this zone
    return false;
}

quint8 CommonFightEngine::getOneSeed(const quint8 &max)
{
    quint16 number=static_cast<quint8>(randomSeeds.at(0));
    if(max!=0)
    {
        number*=max;
        number/=255;
    }
    randomSeeds.remove(0,1);
    return number;
}