/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#include "string/replacements.hpp"

namespace xonotic {

/**
 * \brief Show server connect/disconnect messages
 */
class ConnectionEvents : public melanobot::Handler
{
public:
    ConnectionEvents( const Settings& settings, MessageConsumer* parent )
        : Handler(settings, parent)
    {
        connect = read_string(settings, "connect",
            "Server $(2)$hostname$(-) connected.");
        disconnect = read_string(settings, "disconnect",
            "$(-b)Warning!$(-) Server $(1)$hostname$(-) disconnected.");
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CONNECTED ||
               msg.type == network::Message::DISCONNECTED;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        const auto& str =
            msg.type == network::Message::CONNECTED ? connect : disconnect;
        reply_to(msg, str.replaced(msg.source->pretty_properties()));
        return true;
    }

private:
    string::FormattedString connect;
    string::FormattedString disconnect;
};

/**
 * \brief Shows player join/part messages
 */
class XonoticJoinPart : public melanobot::Handler
{
public:
    XonoticJoinPart( const Settings& settings, MessageConsumer* parent )
        : Handler(settings, parent)
    {
        join = read_string(settings, "join",
            "$(2)+ join$(-): $name $(1)$map$(-) [$(1)$players$(-)/$(1)$max$(-)]");
        part = read_string(settings, "part",
            "$(1)- part$(-): $name $(1)$map$(-) [$(1)$players$(-)/$(1)$max$(-)]");
        bots = settings.get("bots", bots);
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
        auto props = msg.source->pretty_properties(msg.from);

        // Select the right message
        const auto& message = msg.type == network::Message::JOIN ? join : part;

        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString join;
    string::FormattedString part;
    bool        bots = false;
};

/**
 * \brief Shows match start messages
 */
class XonoticMatchStart : public melanobot::Handler
{
public:
    XonoticMatchStart(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        message = read_string(settings, "message",
            "Playing $(dark_cyan)$gametype$(-) on $(1)$map$(-) ($free free slots); join now: $(-b)xonotic +connect $sv_server");
        empty = settings.get("empty", empty);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "gamestart";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto props = msg.source->pretty_properties();

        if ( empty || msg.source->count_users().users )
            reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString message;
    bool empty = false;
};

/**
 * \brief Base class for handlers parsing the Xonotic eventlog
 *
 * It defines a new on_handle which accepts a std::match_result
 */
class ParseEventlog : public melanobot::Handler
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
        if ( std::regex_match(msg.raw, match, regex) )
        {
            return on_handle(msg, match);
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
    : ParseEventlog(R"(^:vote:vcall:(\d+):(.*))", settings, parent)
    {
        message = read_string(settings, "message",
                              "$(4)*$(-) $name$(-) calls a vote for $vote");
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        // Uses pretty_properties with user + "vote"
        user::User user = msg.source->get_user(match[1]);
        auto props = msg.source->pretty_properties(user);
        props["vote"] = msg.source->decode(match[2]);
        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString message;
};

/**
 * \brief Shows :vote:v(yes|no|timeout)
 */
class ShowVoteResult : public ParseEventlog
{
public:
    ShowVoteResult(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:v(yes|no|timeout):(\d+):(\d+):(\d+):(\d+):(-?\d+))", settings, parent)
    {
        message = read_string(settings, "message",
            "$(4)*$(-) vote $message_result: $(dark_green)$yes$(-):$(1)$no$(-)$message_abstain$message_min");
        message_yes = read_string(settings, "message_yes",
            "$(dark_green)passed");
        message_no = read_string(settings, "message_no",
            "$(1)failed");
        message_timeout = read_string(settings, "message_timeout",
            "$(dark_yellow)timed out");
        message_abstain = read_string(settings, "message_abstain",
            ", $abstain_total didn't vote");
        message_min = read_string(settings, "message_min", " ($min needed)");
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        // Message properties
        auto props = msg.source->pretty_properties();
        // result: yes, no, timeout
        std::string result = match[1].str();
        props["result"] = result;
        // yes: # of players who voted yes
        props["yes"] = match[2].str();
        // no: # of players who voted no
        props["no"] = match[3].str();
        // abstain: # of players who voted abstain
        props["abstain"] = match[4].str();
        // novote: # of players who dind't vote
        props["novote"] = match[5].str();
        int abstain_total = std::stoi(match[4])+std::stoi(match[5]);
        // abstain_total: # of players who voted abstain or didn't vote
        props["abstain_total"] = std::to_string(abstain_total);
        // min: minimum # of votes required to pass the vote
        props["min"] = match[6].str();
        int min = std::stoi(match[6]);

        // Sets message_result to one of message_yes, message_no or message_timeout
        if ( result == "yes" )
            props["message_result"] = message_yes.replaced(props);
        else if ( result == "no" )
            props["message_result"] = message_no.replaced(props);
        else
            props["message_result"] = message_timeout.replaced(props);

        // message_abstain is set only if abstain_total is > 0
        if ( abstain_total <= 0 )
            props["message_abstain"] = message_abstain.replaced(props);

        // message_abstain is set only if min is > 0
        if ( min <= 0 )
            props["message_min"] = message_min.replaced(props);

        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    /// Full message
    string::FormattedString message;
    /// Message shown when the result is yes
    string::FormattedString message_yes;
    /// Message shown when the result is no
    string::FormattedString message_no;
    /// Message shown when the result is timeout
    string::FormattedString message_timeout;
    /// Message shown when the $abstain_total is greater than zero
    string::FormattedString message_abstain;
    /// Message shown when $min is greater than zero
    string::FormattedString message_min;
};

/**
 * \brief Shows :vote:vstop
 */
class ShowVoteStop : public ParseEventlog
{
public:
    ShowVoteStop(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vstop:(\d+))", settings, parent)
    {
        message = read_string(settings, "message",
            "$(4)*$(-) $name$(-) stopped the vote");
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        user::User user = msg.source->get_user(match[1]);
        auto props = msg.source->pretty_properties(user);
        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString message;
};

/**
 * \brief Shows :vote:vlogin
 */
class ShowVoteLogin : public ParseEventlog
{
public:
    ShowVoteLogin(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vlogin:(\d+))", settings, parent)
    {
        message = read_string(settings, "message",
            "$(4)*$(-) $name$(-) logged in as $(dark_yellow)master");
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        user::User user = msg.source->get_user(match[1]);
        auto props = msg.source->pretty_properties(user);
        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString message;
};

/**
 * \brief Shows :vote:vdo
 */
class ShowVoteDo : public ParseEventlog
{
public:
    ShowVoteDo(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:vote:vdo:(\d+):(.*))", settings, parent)
    {
        message = read_string(settings, "message",
            "$(4)*$(-) $name$(-) used their master status to do $vote");
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        user::User user = msg.source->get_user(match[1]);
        auto props = msg.source->pretty_properties(user);
        props["vote"] = msg.source->decode(match[2]);
        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString message;
};

/**
 * \brief Shows all the votes
 * \note This will inherit SimpleAction, which isn't needed...
 */
class ShowVotes : public core::PresetGroup
{
public:
    ShowVotes(const Settings& settings, MessageConsumer* parent)
        : PresetGroup({
            "ShowVoteCall",
            "ShowVoteResult",
            "ShowVoteStop",
            "ShowVoteLogin",
            "ShowVoteDo"
        }, settings, parent)
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
        string::FormattedString name;   ///< Player name
        int score = 0;                  ///< Score
        int id = 0;                     ///< Player ID
        PlayerScore(string::FormattedString name, int score, int id)
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
        message = read_string(settings, "message",
            "$(dark_cyan)$gametype$(-) on $(1)$map$(-) ended");
        empty = settings.get("empty", empty);
        show_spectators = settings.get("empty", show_spectators);
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
            handle_end(msg, match);
        else if ( match[2].matched )
            handle_team(match);
        else if ( match[5].matched )
            handle_player(msg.source, match);
        else if ( match[11].matched )
            handle_scores(msg.source, match);
        else if ( match[14].matched )
            handle_labels(match);

        return true;
    }

    /**
     * \brief Handles :end and shows the score list
     */
    void handle_end(const network::Message& msg, const std::smatch& match)
    {
        auto props = msg.source->pretty_properties();

        // End match message
        reply_to(msg, message.replaced(props));

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
                std::stable_sort(it->second.begin(), it->second.end());
            else
                std::stable_sort(it->second.rbegin(), it->second.rend());
        }

        string::FormattedString out;
        color::Color12 color;
        static const int indent = 3;

        // Print team header (team score or spaces if there's no team score)
        auto itt = team_scores.find(team);
        if ( itt != team_scores.end() )
        {
            color = team_colors[team];
            out <<  string::Padding(string::FormattedString() << color << itt->second, indent);
            out << color::nocolor << ") ";
        }
        else
        {
            out << std::string(indent, ' ');
        }

        // Print each player and their score
        for ( unsigned i = 0; i < it->second.size(); i++ )
        {
            if ( team != SPECTATORS )
                out << color << string::FormatFlags::BOLD << it->second[i].score
                    << string::ClearFormatting() << ' ';

            out << it->second[i].name << string::ClearFormatting();

            if ( i < it->second.size() - 1 )
                out << ", ";
        }

        reply_to(msg, std::move(out));
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
                    no_team.insert(no_team.end(), it->second.begin(), it->second.end());
                    it = player_scores.erase(it);
                }
            }
        }

        // Print scores
        auto it = player_scores.begin();
        for ( ++it; it != player_scores.end(); ++it )
            print_scores(msg, it->first);

        // Print spectators
        if ( show_spectators )
            print_scores(msg, SPECTATORS);
    }

    /**
     * \brief Handles :teamscores:see-labels and gathers the team scores
     */
    void handle_team(const std::smatch& match)
    {
        team_scores[melanolib::string::to_int(match[4])] = melanolib::string::to_int(match[3]);
    }

    /**
     * \brief Handles :player:see-labels and gathers the player scores
     */
    void handle_player(network::Connection* conn, const std::smatch& match)
    {
        // Team number (-1 = no team, -2 = spectator)
        int team = melanolib::string::to_int(match[8], 10, SPECTATORS);
        int score = melanolib::string::to_int(match[6]);

        if ( team == NO_TEAM && score == 0 && conn->properties().get("gametype") == "lms" )
            team = SPECTATORS;

        player_scores[team].emplace_back(
            conn->decode(match[10].str()),            // name
            score,
            melanolib::string::to_int(match[9])       // id
        );
    }

    /**
     * \brief Handles :scores, updates gametype and map and initializes the data structures
     */
    void handle_scores(network::Connection* conn, const std::smatch& match)
    {
        conn->properties().put<std::string>("gametype", match[12]);
        conn->properties().put<std::string>("map", match[13]);
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
    string::FormattedString message; ///< Message starting the score list

    std::map<int, std::vector<PlayerScore>>player_scores;  ///< Player Scores (team number => scores)
    std::map<int, int>                     team_scores;    ///< Team Scores (number => score)
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
        message = read_string(settings, "message",
            "$(1)$(-b)SERVER ERROR$(-) $connection ($(-b)$sv_server$(-)) on $(1)$map$(-): $message");
        notify = settings.get("notify", notify);
        if ( notify.empty() )
            throw melanobot::ConfigurationError();
    }


protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        auto props = msg.source->pretty_properties();
        props["connection"] = msg.source->config_name();
        props["message"] = msg.source->decode(match[1]);
        auto reply = message.replaced(props);
        for ( const auto& admin : msg.destination->real_users_in_group(notify) )
        {
            network::OutputMessage out(reply.copy());
            out.target = admin.local_id;
            deliver(msg.destination, out);
        }
        return true;
    }

private:
    string::FormattedString message;
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
            msg.source->properties().put<std::string>(ban_id+".ip", match[4]);
            msg.source->properties().put<std::string>(ban_id+".time", match[5]);
        }
        return true;
    }
};

/**
 * \brief Shows :recordset
 */
class ShowRecordSet : public ParseEventlog
{
public:
    ShowRecordSet(const Settings& settings, MessageConsumer* parent)
    : ParseEventlog(R"(^:recordset:(\d+):(.*))", settings, parent)
    {
        message = read_string(settings, "message",
            "$(4)*$(-) $name$(-) set a new record: $(-b)$time$(-) seconds");
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        user::User user = msg.source->get_user(match[1]);
        auto props = msg.source->pretty_properties(user);
        props["time"] = std::string(match[2]);
        reply_to(msg, message.replaced(props));
        return true;
    }

private:
    string::FormattedString message;
};



} // namespace xonotic
#endif // XONOTIC_HANDLER_LOG_HPP
