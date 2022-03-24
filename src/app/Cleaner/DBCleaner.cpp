/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DBCleaner.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include <vector>
#include <algorithm>
#include <unordered_map>

/*static*/ std::shared_ptr<DBCleaner> DBCleaner::instance()
{
    static auto dbCleaner = std::make_shared<DBCleaner>();
    return dbCleaner;
}

void DBCleaner::Init()
{
    auto result = CharacterDatabase.Query(CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE));
    if (!result)
    {
        LOG_ERROR("cleaner", "> No data in db table `item_instance`");
        return;
    }

    std::vector<int32> guidList;

    do
    {
        auto const& [guid] = result->FetchTuple<int32>();
        guidList.emplace_back(guid);
    } while (result->NextRow());

    auto lastGuid = guidList.back();

    LOG_INFO("cleaner", "> Last item guid {}", lastGuid);

    std::vector<int32> freeIDs;

    for (int32 i = 1; i < lastGuid; i++)
        freeIDs.emplace_back(i);

    for (auto const& itr : guidList)
        std::erase(freeIDs, itr);

    if (freeIDs.empty())
    {
        LOG_INFO("cleaner", "> Found 0 free ids. Very good. Skip clear");
        return;
    }

    std::sort(guidList.begin(), guidList.end(), [](int32 a, int32 b) { return a > b; });

    LOG_INFO("cleaner", "> Found {} free ids. Start replace", freeIDs.size());

    std::vector<std::pair<int32 /*from*/, int32/*to*/>> replaceStore;

    std::size_t count{ 0 };

    for (auto const& itr : guidList)
    {
        if (freeIDs.empty())
            break;

        auto replaceTo = freeIDs.front();
        replaceStore.emplace_back(std::make_pair(itr, replaceTo));
        std::erase(freeIDs, replaceTo);
    }

    count = 0;

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_INSTANCE);
    stmt->SetArguments(3631, 1);
    CharacterDatabase.Execute(stmt);

    std::vector<std::pair<std::string/*table*/, std::string/*column*/>> cleanDBTables =
    {
        { "auctionhouse", "itemguid"},
        { "character_gifts", "item_guid"},
        { "character_inventory", "item"},
        { "guild_bank_item", "item_guid"},
        { "item_loot_storage", "containerGUID"},
        { "item_refund_instance", "item_guid"},
        { "item_soulbound_trade_data", "itemGuid"},
        { "mail_items", "item_guid"},
        { "petition", "petitionguid"},
        { "petition_sign", "petitionguid"}
    };

    for (auto const& [idFrom, idTo] : replaceStore)
    {
        count++;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_INSTANCE);
        stmt->SetArguments(idTo, idFrom);
        CharacterDatabase.Execute(stmt);

        for (auto const& [dbTable, column] : cleanDBTables)
        {
            CharacterDatabase.Execute("UPDATE {0} SET `{1}` = {2} WHERE `{1}` = {3}", dbTable, column, idTo, idFrom);
        }

        LOG_INFO("cleaner", "> {}. Replace item guid from {} to {}", count, idFrom, idTo);
    }
}
