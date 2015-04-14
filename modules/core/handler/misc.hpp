/**
 * \file
 * \brief This file defines handlers which perform miscellaneous tasks
 *
 *
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \license
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
#ifndef HANDLER_MISC
#define HANDLER_MISC

#include "handler/handler.hpp"
#include "math.hpp"

namespace handler {

/**
 * \brief Handler showing licensing information
 * \note Must be enabled to comply to the AGPL
 */
class License : public SimpleAction
{
public:
    License(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("license",settings,parent)
    {
        sources_url = settings.get("url",settings::global_settings.get("website",""));
        help = "Shows licensing information";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,"AGPLv3+ (http://www.gnu.org/licenses/agpl-3.0.html), Sources: "+sources_url);
        return true;
    }

private:
    std::string sources_url;
};

/**
 * \brief Handler showing help on the available handlers
 * \note It is trongly recommended that this is enabled
 * \todo option for inline help (synopsis+message all on the same line)
 */
class Help : public SimpleAction
{
public:
    Help(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("help",settings,parent)
    {
        help = "Shows available commands";
        synopsis += " [command|group]";
        help_group = settings.get("help_group",help_group);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        PropertyTree props;
        bot->populate_properties({"name","help","auth","synopsis","help_group"},
                                 props);

        if ( cleanup(msg, props) )
        {
            PropertyTree result;
            restructure(props,&result);

            auto queried = find(result,msg.message);
            if ( queried )
            {
                string::FormattedString synopsis;
                std::string name = queried->get("name","");

                if ( !name.empty() )
                    synopsis << color::red << name << color::nocolor;

                std::vector<std::string> names;
                gather(*queried, names);
                if ( names.size() > 1 )
                {
                    std::sort(names.begin(),names.end());
                    if ( !synopsis.empty() )
                        synopsis << ": ";
                    synopsis << string::implode(" ",names);
                }

                std::string synopsis_string = queried->get("synopsis","");
                if ( !synopsis_string.empty() )
                {
                    if ( !synopsis.empty() )
                        synopsis << ": ";
                    synopsis << string::FormatterConfig().decode(synopsis_string);
                }

                reply_to(msg,synopsis);

                std::string help = queried->get("help","");
                if ( !help.empty() )
                {
                    reply_to(msg,string::FormattedString() << color::dark_blue
                        << string::FormatterConfig().decode(help));
                }
            }
        }
        else
        {
            reply_to(msg,string::FormattedString() << "Not found: " <<
                string::FormatFlags::BOLD << msg.message);
        }

        return true;
    }

private:
    std::string help_group; ///< Only shows help for members of this group

    /**
     * \brief Remove items the user can't perform
     * \return \b false if \c propeties shall not be considered
     */
    bool cleanup(network::Message& msg, PropertyTree& properties ) const
    {
        if ( msg.source->user_auth(msg.from.local_id,properties.get("auth","")) &&
            properties.get("help_group",help_group) == help_group )
        {
            for ( auto it = properties.begin(); it != properties.end(); )
            {
                if ( !cleanup(msg,it->second) )
                    it = properties.erase(it);
                else
                     ++it;
            }
            return true;
        }
        return false;
    }

    /**
     * \brief removes all internal nodes which don't have a name key
     */
    Optional<PropertyTree> restructure ( const PropertyTree& input, PropertyTree* parent ) const
    {
        Optional<PropertyTree> ret;

        if ( input.get_optional<std::string>("name") )
        {
            ret = PropertyTree();
            parent = &*ret;
        }

        for ( const auto& p : input )
        {
            if ( !p.second.empty() )
            {
                auto child = restructure(p.second,parent);
                if ( child )
                    parent->put_child(p.first,*child);
            }
            else if ( ret && !p.second.data().empty() )
            {
                ret->put(p.first,p.second.data());
            }
        }

        return ret;
    }

    /**
     * \brief Gathers top-level names
     */
    void gather(const PropertyTree& properties, std::vector<std::string>& out) const
    {
        for ( const auto& p : properties )
        {
            auto name = p.second.get_optional<std::string>("name");
            if ( name )
                out.push_back(*name);
            else
                gather(p.second, out);
        }
    }
    /**
     * \brief Searches for a help item with the given name
     */
    const PropertyTree* find ( const PropertyTree& tree,
                               const std::string& name ) const
    {
        auto opt = tree.get_child_optional(name);

        if ( opt && !opt->empty() )
            return &*opt;

        for ( const auto& p : tree )
        {
            auto ptr = find(p.second,name);
            if ( ptr && !ptr->empty() )
                return ptr;
        }

        return nullptr;
    }
};

/**
 * \brief Just repeat what it has been told
 */
class Echo : public SimpleAction
{
public:
    Echo(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("echo",settings,parent)
    {
        help = "Repeats \"Text...\"";
        synopsis += " Text...";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,msg.message);
        return true;
    }
};

/**
 * \brief Shows the server the bot is connected to
 */
class ServerHost : public SimpleAction
{
public:
    ServerHost(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("server",settings,parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,msg.source->description());
        return true;
    }
};

/**
 * \brief Shows one of the given items, at random
 */
class Cointoss : public SimpleAction
{
public:
    Cointoss(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("cointoss",settings,parent)
    {
        auto items = settings.get_optional<std::string>("items");
        if ( items )
            default_items = string::regex_split(*items,",\\s*");
        customizable = settings.get("customizable",customizable);

        help = "Get a random element out of ";
        if ( customizable )
        {
            synopsis += " [item...]";
            help += "the given items";
        }
        else
        {
            help += *items;
        }
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> item_vector;
        if ( customizable )
        {
            item_vector = string::comma_split(msg.message);
            if ( item_vector.size() < 2 )
                item_vector = default_items;
        }
        else
            item_vector = default_items;

        if ( !item_vector.empty() )
            reply_to(msg,item_vector[math::random(item_vector.size()-1)]);

        return true;
    }

private:
    std::vector<std::string> default_items = { "Heads", "Tails" };
    bool                     customizable = true;
};

/**
 * \brief Fixed reply
 */
class Reply : public Handler
{
public:
    Reply(const Settings& settings, handler::HandlerContainer* parent)
        : Handler(settings,parent)
    {
        trigger         = settings.get("trigger","");
        reply           = settings.get("reply","");
        regex           = settings.get("regex",0);
        case_sensitive  = settings.get("case_sensitive",1);
        direct          = settings.get("direct",1);

        if ( trigger.empty() || reply.empty() )
            throw ConfigurationError();

        if ( regex )
        {
            auto flags = std::regex::ECMAScript|std::regex::optimize;
            if ( !case_sensitive )
                flags |= std::regex::icase;
            trigger_regex = std::regex(trigger,flags);
        }
    }

    bool can_handle(const network::Message& msg) const override
    {
        return Handler::can_handle(msg) && !msg.message.empty() &&
            (!direct || msg.direct);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( regex )
        {
            std::smatch match;
            if ( std::regex_match(msg.message,match,trigger_regex) )
            {
                Properties map;
                map["sender"] = msg.from.name;
                for ( unsigned i = 0; i < match.size(); i++ )
                    map[std::to_string(i)] = match[i];
                reply_to(msg,string::replace(reply,map,"%"));
                return true;
            }
        }
        else if ( msg.message == trigger )
        {
            reply_to(msg,reply);
            return true;
        }
        else if ( !case_sensitive &&
            string::strtolower(msg.message) == string::strtolower(trigger) )
        {
            reply_to(msg,reply);
            return true;
        }

        return false;
    }

private:
    std::string trigger;                ///< Trigger pattern
    std::string reply;                  ///< Reply string
    bool        regex{false};           ///< Whether the trigger is a regex
    bool        case_sensitive{true};   ///< Whether matches are case sensitive
    bool        direct{true};           ///< Whether the input message must be direct
    std::regex  trigger_regex;          ///< Regex for the trigger
};

} // namespace handler

#endif // HANDLER_MISC
