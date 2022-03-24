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

#ifndef _CHARACTERDATABASE_H
#define _CHARACTERDATABASE_H

#include "MySQLConnection.h"

enum CharacterDatabaseStatements : uint32
{
    /*  Naming standard for defines:
        {DB}_{SEL/INS/UPD/DEL/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

    CHAR_SEL_ITEM_INSTANCE,
    CHAR_UPD_ITEM_INSTANCE,

    MAX_CHARACTERDATABASE_STATEMENTS
};

class WH_DATABASE_API CharacterDatabaseConnection : public MySQLConnection
{
public:
    typedef CharacterDatabaseStatements Statements;

    //- Constructors for sync and async connections
    CharacterDatabaseConnection(MySQLConnectionInfo& connInfo);
    CharacterDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo);
    ~CharacterDatabaseConnection() override;

    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

#endif
