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

#ifndef _LOG_H_
#define _LOG_H_

#include "Define.h"
#include "StringFormat.h"
#include <unordered_map>

enum class LogLevel : uint8
{
    LOG_LEVEL_DISABLED,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,

    LOG_LEVEL_MAX
};

// For create LogChannel
enum ChannelOptions
{
    CHANNEL_OPTIONS_TYPE,
    CHANNEL_OPTIONS_TIMES,
    CHANNEL_OPTIONS_PATTERN,
    CHANNEL_OPTIONS_OPTION_1,
    CHANNEL_OPTIONS_OPTION_2,
    CHANNEL_OPTIONS_OPTION_3,
    CHANNEL_OPTIONS_OPTION_4,
    CHANNEL_OPTIONS_OPTION_5,
    CHANNEL_OPTIONS_OPTION_6,

    CHANNEL_OPTIONS_MAX
};

enum class FormattingChannelType : uint8
{
    FORMATTING_CHANNEL_TYPE_CONSOLE = 1,
    FORMATTING_CHANNEL_TYPE_FILE
};

// For create Logger
enum LoggerOptions
{
    LOGGER_OPTIONS_LOG_LEVEL,
    LOGGER_OPTIONS_CHANNELS_NAME,

    LOGGER_OPTIONS_UNKNOWN
};

namespace Poco
{
    class FormattingChannel;
    class Logger;
}

class WH_COMMON_API Log
{
private:
    Log();
    ~Log();
    Log(Log const&) = delete;
    Log(Log&&) = delete;
    Log& operator=(Log const&) = delete;
    Log& operator=(Log&&) = delete;

public:
    static Log* instance();

    void Initialize();
    void LoadFromConfig();

    bool ShouldLog(std::string_view type, LogLevel level) const;

    template<typename... Args>
    inline void outMessage(std::string const& filter, LogLevel const level, std::string_view fmt, Args&&... args)
    {
        Write(filter, level, fmt::format(fmt, std::forward<Args>(args)...));
    }

    void Write(std::string_view filter, LogLevel const level, std::string_view message);

private:
    void CreateLoggerFromConfig(std::string const& configLoggerName);
    void CreateChannelsFromConfig(std::string const& logChannelName);
    void ReadLoggersFromConfig();
    void ReadChannelsFromConfig();

    void InitLogsDir();
    void Clear();

    std::string_view GetPositionOptions(std::string_view options, uint8 position, std::string_view _default = {});
    std::string const GetChannelsFromLogger(std::string const& loggerName);

    Poco::FormattingChannel* GetFormattingChannel(std::string const& channelName);
    void AddFormattingChannel(std::string const& channelName, Poco::FormattingChannel* channel);
    Poco::Logger* GetLoggerByType(std::string_view type) const;

    std::string m_logsDir;
    LogLevel highestLogLevel;
    std::unordered_map<std::string, Poco::FormattingChannel*> _channelStore;
};

#define sLog Log::instance()

#define LOG_EXCEPTION_FREE(filterType__, level__, ...) \
    { \
        try \
        { \
            sLog->outMessage(filterType__, level__, fmt::format(__VA_ARGS__)); \
        } \
        catch (const std::exception& e) \
        { \
            sLog->outMessage("server", LogLevel::LOG_LEVEL_ERROR, "Wrong format occurred ({}) at '{}:{}'", \
                e.what(), __FILE__, __LINE__); \
        } \
    }

#define LOG_MSG_BODY(filterType__, level__, ...)                        \
        do {                                                            \
            if (sLog->ShouldLog(filterType__, level__))                 \
                LOG_EXCEPTION_FREE(filterType__, level__, __VA_ARGS__); \
        } while (0)

// Fatal - 1
#define LOG_FATAL(filterType__, ...) \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_FATAL, __VA_ARGS__)

// Critical - 2
#define LOG_CRIT(filterType__, ...) \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_CRITICAL, __VA_ARGS__)

// Error - 3
#define LOG_ERROR(filterType__, ...) \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_ERROR, __VA_ARGS__)

// Warning - 4
#define LOG_WARN(filterType__, ...)  \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_WARNING, __VA_ARGS__)

// Notice - 5
#define LOG_NOTICE(filterType__, ...)  \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_NOTICE, __VA_ARGS__)

// Info - 6
#define LOG_INFO(filterType__, ...)  \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_INFO, __VA_ARGS__)

// Debug - 7
#define LOG_DEBUG(filterType__, ...) \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_DEBUG, __VA_ARGS__)

// Trace - 8
#define LOG_TRACE(filterType__, ...) \
    LOG_MSG_BODY(filterType__, LogLevel::LOG_LEVEL_TRACE, __VA_ARGS__)

#endif // _LOG_H__
