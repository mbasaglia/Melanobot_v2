/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef XONOTIC_HANDLER_LOG_HPP
#define XONOTIC_HANDLER_LOG_HPP

#include "core/handler/group.hpp"

namespace xonotic {

/**
 * \brief Message properties adding properties of the user
 */
inline Properties message_properties(network::Connection* source,
                                     const user::User& user)
{
    string::FormatterConfig fmt;
    Properties props =  source->message_properties();
    props.insert({
        {"name",    source->encode_to(user.name,fmt)},
        {"ip",      user.host},
    });
    return props;
}

/**
 * \brief Show server connect/disconnect messages
 */
class ConnectionEvents : public handler::Handler
{
public:
    ConnectionEvents( const Settings& settings, MessageConsumer* parent )
        : Handler(settings, parent)
    {
        connect = settings.get("connect","Server #2#%host#-# connected.");
        disconnect = settings.get("connect","#-b#Warning!#-# Server #1#%host#-# disconnected.");
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CONNECTED ||
               msg.type == network::Message::DISCONNECTED;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        /// \todo Should the host property be returned from description()?
        Properties props = {
            {"host",msg.source->encode_to(msg.source->properties().get("host"),fmt)},
            {"server",msg.source->server().name()}
        };
        const std::string& str = msg.type == network::Message::CONNECTED ? connect : disconnect;
        reply_to(msg,fmt.decode(string::replace(str,props,"%")));
        return true;
    }

private:
    std::string connect;
    std::string disconnect;
};

/**
 * \brief Shows player join/part messages
 */
class XonoticJoinPart : public handler::Handler
{
public:
    XonoticJoinPart( const Settings& settings, MessageConsumer* parent )
        : Handler(settings, parent)
    {
        join = settings.get("join",join);
        part = settings.get("part",part);
        bots = settings.get("bots",bots);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return ( msg.type == network::Message::JOIN ||
                 msg.type == network::Message::PART ) &&
               ( bots || !msg.from.host.empty() );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        // Get all of the user properties and message properties
        Properties props = msg.from.properties;
        auto overprops = message_properties(msg.source,msg.from);
        props.insert(overprops.begin(),overprops.end());

        // Select the right message
        const std::string& message = msg.type == network::Message::JOIN ? join : part;
        // Format the message and send it
        string::FormatterConfig fmt;
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string join = "#2#+ join#-#: %name #1#%map#-# [#1#%players#-#/#1#%max#-#]";
    std::string part = "#1#- part#-#: %name #1#%map#-# [#1#%players#-#/#1#%max#-#]";
    bool        bots = false;
};

/**
 * \brief Shows match start messages
 */
class XonoticMatchStart : public handler::Handler
{
public:
    XonoticMatchStart( const Settings& settings, MessageConsumer* parent )
        : Handler(settings, parent)
    {
        message = settings.get("message",message);
        empty = settings.get("empty",empty);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "gamestart";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        Properties props = msg.source->message_properties();

        if ( empty || msg.source->count_users().users )
            reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "Playing #dark_cyan#%gametype#-# on #1#%map#-# (%free free slots); join now: #-b#xonotic +connect %sv_server";
    bool empty = false;
};

/**
 * \brief Base class for handlers parsing the Xonotic eventlog
 *
 * It defines a new on_handle which accepts a std::match_result
 */
class ParseEventlog : public handler::Handler
{
public:
    ParseEventlog(std::string regex, const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent),
        regex(std::move(regex), std::regex::ECMAScript|std::regex::optimize)
    {}

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "n";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::smatch match;
        if ( std::regex_match(msg.raw,match,regex) )
        {
            return on_handle(msg,match);
        }
        return false;
    }

    virtual bool on_handle(network::Message& msg, const std::smatch& match) = 0;

private:
    std::regex regex;
};

/**
 * \brief Shows :vote:vcall
 */
class ShowVoteCall : public ParseEventlog
{
public:
    ShowVoteCall(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vcall:(\d+):(.*))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        // Uses message_properties with user + "vote"
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        props["vote"] = msg.source->encode_to(match[2],fmt);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# calls a vote for %vote";
};

/**
 * \brief Shows :vote:v(yes|no|timeout)
 */
class ShowVoteResult : public ParseEventlog
{
public:
    ShowVoteResult(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:v(yes|no|timeout):(\d+):(\d+):(\d+):(\d+):(-?\d+))",settings,parent)
    {
        message = settings.get("message",message);
        message_yes = settings.get("message_yes",message_yes);
        message_no = settings.get("message_no",message_no);
        message_timeout = settings.get("message_timeout",message_timeout);
        message_abstain = settings.get("message_abstain",message_abstain);
        message_min = settings.get("message_min",message_min);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        // Message properties
        string::FormatterConfig fmt;
        Properties props = msg.source->message_properties();
        // result: yes, no, timeout
        props["result"] = match[1];
        // yes: # of players who voted yes
        props["yes"] = match[2];
        // no: # of players who voted no
        props["no"] = match[3];
        // abstain: # of players who voted abstain
        props["abstain"] = match[4];
        // novote: # of players who dind't vote
        props["novote"] = match[5];
        int abstain_total = std::stoi(match[4])+std::stoi(match[5]);
        // abstain_total: # of players who voted abstain or didn't vote
        props["abstain_total"] = std::to_string(abstain_total);
        // min: minimum # of votes required to pass the vote
        props["min"] = match[6];
        int min = std::stoi(match[6]);

        // Sets message_result to one of message_yes, message_no or message_timeout
        if ( props["result"] == "yes" )
            props["message_result"] = string::replace(message_yes,props,"%");
        else if ( props["result"] == "no" )
            props["message_result"] = string::replace(message_no,props,"%");
        else
            props["message_result"] = string::replace(message_timeout,props,"%");

        // message_abstain is set only if abstain_total is > 0
        props["message_abstain"] = abstain_total <= 0 ? "" :
            string::replace(message_abstain,props,"%");

        // message_abstain is set only if min is > 0
        props["message_min"] = min <= 0 ? "" :
            string::replace(message_min,props,"%");

        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    /// Full message
    std::string message         = "#4#*#-# vote %message_result: #dark_green#%yes#-#:#1#%no#-#%message_abstain%message_min";
    /// Message shown when the result is yes
    std::string message_yes     = "#dark_green#passed";
    /// Message shown when the result is no
    std::string message_no      = "#1#failed";
    /// Message shown when the result is timeout
    std::string message_timeout = "#dark_yellow#timed out";
    /// Message shown when the %abstain_total is greater than zero
    std::string message_abstain = ", %abstain_total didn't vote";
    /// Message shown when %min is greater than zero
    std::string message_min     = " (%min needed)";
};

/**
 * \brief Shows :vote:vstop
 */
class ShowVoteStop : public ParseEventlog
{
public:
    ShowVoteStop(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vstop:(\d+))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# stopped the vote";
};

/**
 * \brief Shows :vote:vlogin
 */
class ShowVoteLogin : public ParseEventlog
{
public:
    ShowVoteLogin(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vlogin:(\d+))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# logged in as #dark_yellow#master";
};

/**
 * \brief Shows :vote:vdo
 */
class ShowVoteDo : public ParseEventlog
{
public:
    ShowVoteDo(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vdo:(\d+):(.*))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        props["vote"] = msg.source->encode_to(match[2], fmt);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# used their master status to do %vote";
};

/**
 * \brief Shows all the votes
 * \note This will inherit SimpleAction, which isn't needed...
 */
class ShowVotes : public handler::PresetGroup
{
public:
    ShowVotes(const Settings& settings, MessageConsumer* parent)
        : PresetGroup({
            "ShowVoteCall",
            "ShowVoteResult",
            "ShowVoteStop",
            "ShowVoteLogin",
            "ShowVoteDo"
        },settings,parent)
    {}
};

/**
 * \brief Shows match scores
 * \todo Could use parameterized score list messages
 */
class XonoticMatchScore : public ParseEventlog
{
private:
    /**
     * \brief Struct for player score
     */
    struct PlayerScore
    {
        std::string name;       ///< Player name
        int score = 0;          ///< Score
        int id = 0;             ///< Player ID
        PlayerScore(std::string name, int score, int id)
            : name(std::move(name)), score(score), id(id) {}

        bool operator< (const PlayerScore& o) const noexcept { return score < o.score; }
        bool operator> (const PlayerScore& o) const noexcept { return score > o.score; }
    };

public:
    XonoticMatchScore( const Settings& settings, MessageConsumer* parent )
        : ParseEventlog(
            ":(?:"
            // 1
            "(end)"
            // 2                     score=3                id=4
            "|(teamscores:see-labels:(-?\\d+)(?:-|[0-9,])*:(\\d+))"
            // 5                  score=6              time=7 team=8   id=9 name=10
            "|(player:see-labels:(-?\\d+)(?:-|[0-9,])*:(\\d+):([^:]+):(\\d+):(.*))"
            // 11   gametype=12 map=13
            "|(scores:([a-z]+)_(.*)):\\d+"
            // 14           primary=15 sort=16
            "|(labels:player:([^[,<!]*)(<)?!!,.*)"
            ")",
            settings, parent
        )
    {
        message = settings.get("message",message);
        empty = settings.get("empty",empty);
    }

    /**
     * \todo maybe this check isn't needed (empty check can be deferred to :end)
     */
    bool can_handle(const network::Message& msg) const override
    {
        return !msg.raw.empty() && msg.raw[0] == ':' &&
            (empty || msg.source->count_users().users);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        /**
         * \note The order in which the messages are received is the following:
         *      * :scores
         *      * :labels
         *      * :player:see-labels
         *      * :teamscores:see-labels
         *      * :end
         */
        if ( match[1].matched )
            handle_end(msg,match);
        else if ( match[2].matched )
            handle_team(match);
        else if ( match[5].matched )
            handle_player(msg.source,match);
        else if ( match[11].matched )
            handle_scores(msg.source,match);
        else if ( match[14].matched )
            handle_labels(match);

        return true;
    }

    /**
     * \brief Handles :end and shows the score list
     */
    void handle_end(const network::Message& msg, const std::smatch& match)
    {
        string::FormatterConfig fmt;
        Properties props = msg.source->message_properties();

        // End match message
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));

        // Shows scores only if there are some scores to show
        if ( !player_scores.empty() || !team_scores.empty() )
            print_scores(msg);

        // Clear all state
        player_scores.clear();
        team_scores.clear();
        sort_reverse = false;
    }

    /**
     * \brief Prints the score for all the players in a given team
     */
    void print_scores(const network::Message& msg, int team)
    {
        // Check the team has scores
        auto it = player_scores.find(team);
        if ( it == player_scores.end() || it->second.empty() )
            return;

        // Sort players by score
        if ( team != SPECTATORS )
        {
            if ( sort_reverse )
                std::stable_sort(it->second.begin(),it->second.end());
            else
                std::stable_sort(it->second.rbegin(),it->second.rend());
        }

        string::FormattedString out;
        color::Color12 color;
        static const int indent = 3;

        // Print team header (team score or spaces if there's no team score)
        auto itt = team_scores.find(team);
        if ( itt != team_scores.end() )
        {
            color = team_colors[team];
            out << color << itt->second;
            out.pad(indent);
            out << color::nocolor << ") ";
        }
        else
        {
            out << std::string(indent,' ');
        }

        // Print each player and their score
        for ( unsigned i = 0; i < it->second.size(); i++ )
        {
            if ( team != SPECTATORS )
                out << color << string::FormatFlags::BOLD << it->second[i].score
                    << string::ClearFormatting() << ' ';

            out << msg.source->decode(it->second[i].name)
                << string::ClearFormatting();

            if ( i < it->second.size() - 1 )
                out << ", ";
        }

        reply_to(msg, out);
    }

    /**
     * \brief Prints the score list
     */
    void print_scores(const network::Message& msg)
    {
        // Don't show scores if there are only spectators
        if ( !show_spectators && player_scores.size() < 2 )
            return;

        // For some reason deathmatch assigns players to random teams,
        // this ensures they are places in NO_TEAM
        if ( team_scores.empty() )
        {
            auto& no_team = player_scores[NO_TEAM];
            for (auto it = player_scores.begin(); it != player_scores.end(); )
            {
                if ( it->first < 0 )
                {
                    ++it;
                }
                else
                {
                    no_team.insert(no_team.end(),it->second.begin(),it->second.end());
                    it = player_scores.erase(it);
                }
            }
        }

        // Print scores
        auto it = player_scores.begin();
        for ( ++it; it != player_scores.end(); ++it )
            print_scores(msg,it->first);

        // Print spectators
        if ( show_spectators )
            print_scores(msg,SPECTATORS);
    }

    /**
     * \brief Handles :teamscores:see-labels and gathers the team scores
     */
    void handle_team(const std::smatch& match)
    {
        team_scores[string::to_int(match[4])] = string::to_int(match[3]);
    }

    /**
     * \brief Handles :player:see-labels and gathers the player scores
     */
    void handle_player(network::Connection* conn, const std::smatch& match)
    {
        // Team number (-1 = no team, -2 = spectator)
        int team = string::to_int(match[8],10,SPECTATORS);
        int score = string::to_int(match[6]);

        if ( team == NO_TEAM && score == 0 && conn->properties().get("gametype") == "lms" )
            team = SPECTATORS;

        player_scores[team].emplace_back(
            match[10],                     // name
            score,
            string::to_int(match[9])       // id
        );
    }

    /**
     * \brief Handles :scores, updates gametype and map and initializes the data structures
     */
    void handle_scores(network::Connection* conn, const std::smatch& match)
    {
        conn->properties().put<std::string>("gametype",match[12]);
        conn->properties().put<std::string>("map",match[13]);
        player_scores = { {SPECTATORS, {}} };
        team_scores.clear();
        sort_reverse = false;
    }

    /**
     * \brief Handles :labels:player and selects the sorting order
     */
    void handle_labels(const std::smatch& match)
    {
        sort_reverse = match[16].matched;
    }

private:
    bool empty = false;         ///< Whether to show the scores on an empty/bot-only match
    bool show_spectators = true; ///< Whether to show spectators on the score list
    std::string message = "#dark_cyan#%gametype#-# on #1#%map#-# ended"; ///< Message starting the score list

    std::map<int,std::vector<PlayerScore>>player_scores;  ///< Player Scores (team number => scores)
    std::map<int,int>                     team_scores;    ///< Team Scores (number => score)
    bool                                  sort_reverse{0};///< Whether a low score is better

    /// Map team numbers to their color
    std::unordered_map<int, color::Color12> team_colors {
        {  5, color::red    },
        { 14, color::blue   },
        { 13, color::yellow },
        { 10, color::magenta}
    };

    /// Quick constants for teams
    enum {
        NO_TEAM = -1,
        SPECTATORS = -2,
    };
};

/**
 * \brief Notifies admins that a severe error has occurred in the Xonotic server
 */
class XonoticHostError : public ParseEventlog
{
public:
    XonoticHostError( const Settings& settings, MessageConsumer* parent )
        : ParseEventlog( "Host_Error: (.*)", settings, parent )
    {
        message = settings.get("message",message);
        notify = settings.get("notify",notify);
        if ( notify.empty() )
            throw ConfigurationError();
    }


protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        auto props = msg.source->message_properties();
        props["connection"] = msg.source->config_name();
        props["message"] = msg.source->encode_to(match[1], fmt);
        for ( const auto& admin : msg.destination->real_users_in_group(notify) )
        {
            network::OutputMessage out(fmt.decode(string::replace(message,props,"%")));
            out.target = admin.local_id;
            deliver(msg.destination,out);
        }
        return true;
    }

private:
    std::string message = "#1##-b#SERVER ERROR#-# %connection (#-b#%sv_server#-#) on #1#%map#-#: %message";
    std::string notify; ///< Group to be notified
};

/**
 * \brief This class stores the ban list in the connection
 */
class XonoticUpdateBans : public ParseEventlog
{
public:
    XonoticUpdateBans(const Settings& settings, MessageConsumer* parent)
        : ParseEventlog(
            "(\\^2Listing all existing active bans:)|"// 1
            //         2  banid=3     ip=4                            time=5
            R"regex(\s*(#([0-9]+): (\S+) is still banned for (inf|[0-9]+)\S* seconds))regex",
            //R"regex(\s*(#([0-9]+): ([:./0-9]+) is still banned for (inf|[0-9]+)(?:\.[0-9]+)? seconds))regex",
            settings, parent )
    {}

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        if ( match[1].matched )
        {
            msg.source->properties().erase("banlist");
        }
        else if ( match[2].matched )
        {
            std::string ban_id = "banlist."+std::string(match[3]);
            msg.source->properties().put<std::string>(ban_id+".ip",match[4]);
            msg.source->properties().put<std::string>(ban_id+".time",match[5]);
        }
        return true;
    }
};

} // namespace xonotic
#endif // XONOTIC_HANDLER_LOG_HPP
