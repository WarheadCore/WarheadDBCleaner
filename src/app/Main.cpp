/*
 * This file is part of the WarheadApp Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config.h"
#include "DeadlineTimer.h"
#include "IoContext.h"
#include "StopWatch.h"
#include "GitRevision.h"
#include "Log.h"
#include "Logo.h"
#include "MySQLThreading.h"
#include "IPLocation.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "DBCleaner.h"
#include <boost/version.hpp>

#ifndef _WARHEAD_DB_CLEANER_CONFIG
#define _WARHEAD_DB_CLEANER_CONFIG "WarheadDBCleaner.conf"
#endif

bool StartDB();
void StopDB();
void KeepDatabaseAliveHandler(std::weak_ptr<Warhead::Asio::DeadlineTimer> dbPingTimerRef, boost::system::error_code const& error);

int main(int argc, char** argv)
{
    // Command line parsing to get the configuration file name
    std::string configFile = sConfigMgr->GetConfigPath() + std::string(_WARHEAD_DB_CLEANER_CONFIG);
    int count = 1;

    while (count < argc)
    {
        if (strcmp(argv[count], "-c") == 0)
        {
            if (++count >= argc)
            {
                printf("Runtime-Error: -c option requires an input argument\n");
                return 1;
            }
            else
                configFile = argv[count];
        }
        ++count;
    }

    if (!sConfigMgr->LoadAppConfigs(configFile))
        return 1;

    // Init logging
    sLog->Initialize();

    Warhead::Logo::Show("dbcleaner",
        [](std::string_view text)
        {
            LOG_INFO("server", text);
        },
        []()
        {
            LOG_INFO("server", "> Using configuration file:       {}", sConfigMgr->GetFilename());
            LOG_INFO("server", "> Using Boost version:            {}.{}.{}", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);
        }
    );

    // Initialize the database connection
    if (!StartDB())
        return 1;

    std::shared_ptr<void> dbHandle(nullptr, [](void*) { StopDB(); });

    //std::shared_ptr<Warhead::Asio::IoContext> ioContext = std::make_shared<Warhead::Asio::IoContext>();

    LOG_INFO("server", "{} (dbcleaner-daemon) ready...", GitRevision::GetFullVersion());

    sDBCleaner->Init();

    //// Enabled a timed callback for handling the database keep alive ping
    //std::shared_ptr<Warhead::Asio::DeadlineTimer> dbPingTimer = std::make_shared<Warhead::Asio::DeadlineTimer>(*ioContext);
    //dbPingTimer->expires_from_now(boost::posix_time::minutes(30));
    //dbPingTimer->async_wait(std::bind(&KeepDatabaseAliveHandler, std::weak_ptr<Warhead::Asio::DeadlineTimer>(dbPingTimer), std::placeholders::_1));

    //// Start the io service worker loop
    //ioContext->run();

    LOG_INFO("server", "Halting process...");

    //dbPingTimer->cancel();

    return 0;
}

/// Initialize connection to the database
bool StartDB()
{
    MySQL::Library_Init();

    // Load databases
    // NOTE: While authserver is singlethreaded you should keep synch_threads == 1.
    // Increasing it is just silly since only 1 will be used ever.
    DatabaseLoader loader("server");
    loader
        .AddDatabase(CharacterDatabase, "Character")
        .AddDatabase(WorldDatabase, "World");

    if (!loader.Load())
        return false;

    LOG_INFO("server", "Started database connection pool.");
    return true;
}

/// Close the connection to the database
void StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    MySQL::Library_End();
}

void KeepDatabaseAliveHandler(std::weak_ptr<Warhead::Asio::DeadlineTimer> dbPingTimerRef, boost::system::error_code const& error)
{
    if (!error)
    {
        if (std::shared_ptr<Warhead::Asio::DeadlineTimer> dbPingTimer = dbPingTimerRef.lock())
        {
            LOG_INFO("server", "Ping MySQL to keep connection alive");
            CharacterDatabase.KeepAlive();
            WorldDatabase.KeepAlive();

            dbPingTimer->expires_from_now(boost::posix_time::minutes(30));
            dbPingTimer->async_wait(std::bind(&KeepDatabaseAliveHandler, dbPingTimerRef, std::placeholders::_1));
        }
    }
}
