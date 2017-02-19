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
#ifndef LOCKED_PROPERTIES_HPP
#define LOCKED_PROPERTIES_HPP

#include "settings.hpp"
#include "locking_reference.hpp"
#include <mutex>

class LockedProperties : public LockingReferenceBase<PropertyTree>
{
public:
    typedef PropertyTree::path_type path_type;

    using LockingReferenceBase::LockingReferenceBase;

    /**
     * \brief Returns a property as a string, empty string if not found
     */
    std::string get(const path_type& property)
    {
        auto l = lock();
        return referenced->get(property, "");
    }

    /**
     * \brief Returns a property converted to the given type
     */
    template<class T>
        auto get(const path_type& property, T&& default_value)
    {
        auto l = lock();
        return referenced->get(property, std::forward<T>(default_value));
    }

    /**
     * \brief Sets a property
     */
    template<class T>
        void put(const path_type& property, T&& value)
    {
        auto l = lock();
        referenced->put(property, std::forward<T>(value));
    }

    /**
     * \brief Erase a property (and all of its children)
     */
    void erase(path_type path)
    {
        auto l = lock();

        auto node = referenced;
        auto child = node->not_found();
        while ( !path.empty() )
        {
            child = node->find(path.reduce());
            if ( child == node->not_found() )
                return;
            node = &child->second;
        }
        if ( child != node->not_found() )
            node->erase(node->to_iterator(child));
    }

    /**
     * \brief Get a copy of a child property
     */
    PropertyTree get_child(const path_type& property)
    {
        auto l = lock();
        return referenced->get_child(property, {});
    }

    PropertyTree copy()
    {
        auto l = lock();
        return *referenced;
    }
};


#endif // LOCKED_PROPERTIES_HPP
