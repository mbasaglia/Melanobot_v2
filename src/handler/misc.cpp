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
#include "handler.hpp"

namespace handler {

/**
 * \brief Handler showing licensing information
 * \note Must be enabled to comply to the AGPL
 */
class License : public SimpleAction
{
public:
    License(const Settings& settings, Melanobot* bot)
        : SimpleAction("license",settings,bot)
    {
        sources_url = settings.get("url",Settings::global_settings.get("website",""));
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
REGISTER_HANDLER(License,License);

/**
 * \brief Handler showing help on the available handlers
 * \note It is trongly recommended that this is enabled
 * \todo option for inline help (synopsis+message all on the same line)
 */
class Help : public SimpleAction
{
public:
    Help(const Settings& settings, Melanobot* bot)
        : SimpleAction("help",settings,bot)
    {
        help = "Shows available commands";
        synopsis += " [command|group]";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        PropertyTree props;
        /// \todo Discard commands available for other connections
        bot->populate_properties({"name","help","auth","synopsis"},props);

        if ( cleanup(msg, props) )
        {
            PropertyTree result;
            restructure(props,&result);

            auto queried = find(result,msg.message);
            if ( queried )
            {
                string::FormattedStream synopsis;
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
                    synopsis << color::gray << synopsis_string;
                }

                reply_to(msg,synopsis.str());

                std::string help = queried->get("help","");
                if ( !help.empty() )
                {
                    reply_to(msg,(string::FormattedStream()
                        << color::dark_blue << help).str());
                }
            }
        }
        else
        {
            reply_to(msg,(string::FormattedStream() << "Not found: " <<
                string::FormatFlags::BOLD << msg.message).str());
        }

        return true;
    }

private:
    /**
     * \brief Remove items the user can't perform
     * \return \b false if \c propeties shall not be considered
     */
    bool cleanup(network::Message& msg, PropertyTree& properties ) const
    {
        if ( msg.source->user_auth(msg.from,properties.get("auth","")) )
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
    boost::optional<PropertyTree> restructure ( const PropertyTree& input, PropertyTree* parent ) const
    {
        boost::optional<PropertyTree> ret;

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
    boost::optional<const PropertyTree&> find ( const PropertyTree& tree,
                                                const std::string& name ) const
    {
        auto opt = tree.get_child_optional(name);

        if ( opt && !opt->empty() )
            return opt;

        for ( const auto& p : tree )
        {
            opt = find(p.second,name);
            if ( opt && !opt->empty() )
                return opt;
        }

        return {};
    }
};
REGISTER_HANDLER(Help,Help);


/**
 * \brief Just repeat what it has been told
 */
class Echo : public SimpleAction
{
public:
    Echo(const Settings& settings, Melanobot* bot)
        : SimpleAction("echo",settings,bot)
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
REGISTER_HANDLER(Echo,Echo);

} // namespace handler
