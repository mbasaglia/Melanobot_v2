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
#ifndef WEB_REST_HPP
#define WEB_REST_HPP

#include "settings.hpp"

namespace web {
namespace rest {

enum class GetPolicy {
    ONCE    = 0,
    LAZY    = 1,
    DYNAMIC = 2,
};
enum class PutPolicy {
    ONCE    = 0,
    DYNAMIC = 2,
    DISCARD = 3,
};
enum class DataFormat {
    JSON    = 0,
    XML     = 1,
};

/**
 * \brief Base class for Api and Object
 */
class RestBase
{
public:

    std::string url_base() const;
    std::string url_suffix() const;

    GetPolicy get_policy() const noexcept;
    std::string get_format() const;
    PutPolicy put_policy() const noexcept;
    std::string put_format() const;

    DataFormat data_format() const noexcept;

private:
};

/**
 * \brief An object representing an interface to a REST API
 */
class Api : public RestBase
{
    friend class RestObject;

public:
    /**
     * \brief Returns an object of the given type and id
     */
    RestObject* object_get(const std::string& type, const std::string& id);

protected:
    /**
     * \brief Low-level operation to get a resource
     * \param url       URL of the GET call
     * \return The response parsed as for data_format()
     */
    PropertyTree object_get(const std::string& url);
    /**
     * \brief Low-level operation to put a resource
     * \param url       URL of the PUT call
     * \param object    Properties to be PUT
     * \return The response parsed as for data_format()
     */
    PropertyTree object_put(const std::string& url, const PropertyTree& object);
    /**
     * \brief Low-level operation to delete a resource
     * \param url       URL of the DELETE call
     * \return The response parsed as for data_format()
     */
    PropertyTree object_delete(const std::string& url);

};

/**
 * \brief An object intefacing to a REST resource
 */
class Object : public RestBase
{
public:
    RestObject(RestApi*         api,    ///< Api managing this object
               std::string      type,   ///< Resource type
               std::string      id,     ///< Resource identifier
               GetPolicy        get_policy = GetPolicy::ONCE,
               PutPolicy        put_policy = PutPolicy::ONCE
              );

    /**
     * \brief Api managing this object
     */
    RestApi* api() const noexcept;

    /**
     * \brief Resource type
     */
    std::string type() const;
    /**
     * \brief Resource identifier
     */
    std::string id() const;

    /**
     * \brief Read the properties from the server
     */
    void object_get();
    /**
     * \brief Write the properties to the server
     */
    void object_put();
    /**
     * \brief Delete from the server
     */
    void object_delete();

    /**
     * \brief Discard changes made to this object
     *
     * Changes put_policy() to PutPolicy::DISCARD
     */
    void discard();

    /**
     * \brief Returns the value of the given property
     *
     * If get_policy() is GetPolicy::ONCE, all the properties have been already
     * loaded and it returns the value directly.
     *
     * If get_policy() is GetPolicy::LAZY and the properties have not yet
     * been loaded, calls object_get(), which might be slow.
     *
     * If get_policy() is GetPolicy::DYNAMIC, always calls object_get(),
     * which might be slow.
     */
    template<class T>
        T get(const std::string& property);
    /**
     * \brief Returns the value of the given property
     *
     * If get_policy() is GetPolicy::ONCE, all the properties have been already
     * loaded and it returns the value directly.
     *
     * If get_policy() is GetPolicy::LAZY and the properties have not yet
     * been loaded throws an exception.
     *
     * If get_policy() is GetPolicy::DYNAMIC, throws an exception.
     *
     * \throws TODO When properties aren't available
     */
    template<class T>
        T get(const std::string& property) const;

    /**
     * \brief Sets the value of a property
     *
     * If put_policy() is PutPolicy::DYNAMIC, calls object_put( ,
     * which might be slow.
     **/
    template<class T>
        void put(const std::string& property, T&& value);

    /**
     * \brief Returns all the properties
     *
     * If get_policy() is GetPolicy::ONCE, all the properties have been already
     * loaded and it returns the value directly.
     *
     * If get_policy() is GetPolicy::LAZY and the properties have not yet
     * been loaded, calls object_get(), which might be slow.
     *
     * If get_policy() is GetPolicy::DYNAMIC, always calls object_get(),
     * which might be slow.
     */
    PropertyTree properties();
    /**
     * \brief Returns all the properties
     *
     * If get_policy() is GetPolicy::ONCE, all the properties have been already
     * loaded and it returns the value directly.
     *
     * If get_policy() is GetPolicy::LAZY and the properties have not yet
     * been loaded throws an exception.
     *
     * If get_policy() is GetPolicy::DYNAMIC, throws an exception.
     *
     * \throws TODO When properties aren't available
     */
    PropertyTree properties() const;
};


}} // namespace web::rest
#endif // WEB_REST_HPP
