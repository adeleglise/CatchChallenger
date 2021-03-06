#include "Client.h"

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLib.h"
#include "PreparedDBQuery.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

void Client::addWarehouseObject(const uint16_t &item,const uint32_t &quantity,const bool databaseSync)
{
    if(public_and_private_informations.warehouse_items.find(item)!=public_and_private_informations.warehouse_items.cend())
    {
        public_and_private_informations.warehouse_items[item]+=quantity;
        if(databaseSync)
            updateObjectInWarehouseDatabase();
    }
    else
    {
        public_and_private_informations.warehouse_items[item]=quantity;
        if(databaseSync)
            updateObjectInWarehouseDatabase();
    }
}

uint32_t Client::removeWarehouseObject(const uint16_t &item,const uint32_t &quantity,const bool databaseSync)
{
    if(public_and_private_informations.warehouse_items.find(item)!=public_and_private_informations.warehouse_items.cend())
    {
        if(public_and_private_informations.warehouse_items.at(item)>quantity)
        {
            public_and_private_informations.warehouse_items[item]-=quantity;
            if(databaseSync)
                updateObjectInWarehouseDatabase();
            return quantity;
        }
        else
        {
            const uint32_t removed_quantity=public_and_private_informations.warehouse_items.at(item);
            public_and_private_informations.warehouse_items.erase(item);
            if(databaseSync)
                updateObjectInWarehouseDatabase();
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::addWarehouseCash(const uint64_t &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.warehouse_cash+=cash;
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_warehouse_cash.asyncWrite({
                std::to_string(public_and_private_informations.warehouse_cash),
                std::to_string(character_id)
                });
}

void Client::removeWarehouseCash(const uint64_t &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.warehouse_cash-=cash;
    public_and_private_informations.warehouse_cash+=cash;
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_warehouse_cash.asyncWrite({
                std::to_string(public_and_private_informations.warehouse_cash),
                std::to_string(character_id)
                });
}

void Client::wareHouseStore(const int64_t &cash, const std::vector<std::pair<uint16_t, int32_t> > &items, const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters)
{
    if(!wareHouseStoreCheck(cash,items,withdrawMonsters,depositeMonsters))
        return;
    {
        unsigned int index=0;
        while(index<items.size())
        {
            const std::pair<uint16_t, int32_t> &item=items.at(index);
            if(item.second>0)
            {
                removeWarehouseObject(item.first,item.second);
                addObject(item.first,item.second);
            }
            if(item.second<0)
            {
                removeObject(item.first,-item.second);
                addWarehouseObject(item.first,-item.second);
            }
            index++;
        }
    }
    //validate the change here
    if(cash>0)
    {
        removeWarehouseCash(cash);
        addCash(cash);
    }
    if(cash<0)
    {
        removeCash(-cash);
        addWarehouseCash(-cash);
    }
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.warehouse_playerMonster.size())
            {
                const PlayerMonster &playerMonsterinWarehouse=public_and_private_informations.warehouse_playerMonster.at(sub_index);
                if(playerMonsterinWarehouse.id==withdrawMonsters.at(index))
                {
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_position_and_place.asyncWrite({
                                           std::to_string(public_and_private_informations.playerMonster.size()),
                                           "1",
                                           std::to_string(playerMonsterinWarehouse.id)
                                           });
                    public_and_private_informations.playerMonster.push_back(public_and_private_informations.warehouse_playerMonster.at(sub_index));
                    public_and_private_informations.warehouse_playerMonster.erase(public_and_private_informations.warehouse_playerMonster.cbegin()+sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonsterOnPlayer=public_and_private_informations.playerMonster.at(sub_index);
                if(playerMonsterOnPlayer.id==depositeMonsters.at(index))
                {
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_position_and_place.asyncWrite({
                                           std::to_string(public_and_private_informations.warehouse_playerMonster.size()),
                                           "2",
                                           std::to_string(playerMonsterOnPlayer.id)
                                           });
                    public_and_private_informations.warehouse_playerMonster.push_back(public_and_private_informations.playerMonster.at(sub_index));
                    public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.cbegin()+sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    if(depositeMonsters.size()>0 || withdrawMonsters.size()>0)
    {
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheDisconnexion)
            saveMonsterStat(public_and_private_informations.playerMonster.back());
//        updateMonsterInDatabase();
//        updateMonsterInWarehouseDatabase();
    }
    if(!items.empty())
    {
        updateObjectInDatabase();//do above in bad way, improve this
        updateObjectInWarehouseDatabase();//do above in bad way, improve this
    }
}

bool Client::wareHouseStoreCheck(const int64_t &cash, const std::vector<std::pair<uint16_t, int32_t> > &items, const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters)
{
    //check all
    if((cash>0 && (int64_t)public_and_private_informations.warehouse_cash<cash) || (cash<0 && (int64_t)public_and_private_informations.cash<-cash))
    {
        errorOutput("cash transfer is wrong");
        return false;
    }
    {
        unsigned int index=0;
        while(index<items.size())
        {
            const std::pair<uint16_t, int32_t> &item=items.at(index);
            if(item.second>0)
            {
                if(public_and_private_informations.warehouse_items.find(item.first)==public_and_private_informations.warehouse_items.cend())
                {
                    errorOutput("warehouse item transfer is wrong due to missing");
                    return false;
                }
                if((int64_t)public_and_private_informations.warehouse_items.at(item.first)<item.second)
                {
                    errorOutput("warehouse item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            if(item.second<0)
            {
                if(public_and_private_informations.items.find(item.first)==public_and_private_informations.items.cend())
                {
                    errorOutput("item transfer is wrong due to missing");
                    return false;
                }
                if((int64_t)public_and_private_informations.items.at(item.first)<-item.second)
                {
                    errorOutput("item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            index++;
        }
    }
    int count_change=0;
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.warehouse_playerMonster.size())
            {
                if(public_and_private_informations.warehouse_playerMonster.at(sub_index).id==withdrawMonsters.at(index))
                {
                    count_change++;
                    break;
                }
                sub_index++;
            }
            if(sub_index==public_and_private_informations.warehouse_playerMonster.size())
            {
                errorOutput("no monster to withdraw");
                return false;
            }
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.size())
            {
                if(public_and_private_informations.playerMonster.at(sub_index).id==depositeMonsters.at(index))
                {
                    count_change--;
                    break;
                }
                sub_index++;
            }
            if(sub_index==public_and_private_informations.playerMonster.size())
            {
                errorOutput("no monster to deposite");
                return false;
            }
            index++;
        }
    }
    if((public_and_private_informations.playerMonster.size()+count_change)>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        errorOutput("have more monster to withdraw than the allowed");
        return false;
    }
    return true;
}
