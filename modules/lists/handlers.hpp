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
#ifndef TODO_HANDLERS_HPP
#define TODO_HANDLERS_HPP

#include "core/handler/group.hpp"
#include "network/async_service.hpp"

/**
 * \brief Namespace for handlers managing lists
 */
namespace lists {

/**
 * \brief Simple manager for a fixed list
 */
class FixedList : public core::AbstractList {
public:
    FixedList(const Settings& settings, MessageConsumer* parent)
        : AbstractList("list", true, settings, parent),
        list_id("lists."+trigger)
    {
        network::require_service("storage");
    }

    bool add(const std::string& element) override
    {
        raw_set(raw_get()+'\n'+element);
        return true;
    }

    std::vector<std::string> elements() const override
    {
        return string::char_split(raw_get(),'\n');
    }

    bool remove(const std::string& element) override
    {
        auto list = elements();
        auto it = std::remove(list.begin(), list.end(), element);
        if ( it == list.end() )
            return false;
        list.erase(it, list.end());
        raw_set(string::implode("\n",list));
        return true;
    }

    bool clear() override
    {
        raw_set("");
        return true;
    }

    std::string get_property(const std::string& name) const override
    {
        if ( name == "list_name" )
            return trigger;
        return AbstractList::get_property(name);
    }

protected:
    /**
     * \brief Get the raw value of the list as from storage
     */
    std::string raw_get() const
    {
        return network::service("storage")
            ->query({"maybe_get", list_id, {""}}).contents;
    }

    /**
     * \brief Sets the raw value (elements separated by tabs)
     */
    void raw_set(const std::string& element_string) const
    {
        network::service("storage")
            ->query({"put", list_id, {element_string}});
    }


private:
    std::string list_id; ///< List name in the storage system
};

} // namespace lists
#endif // TODO_HANDLERS_HPP
