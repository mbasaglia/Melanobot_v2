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
#ifndef MELANOMODULE_HPP
#define MELANOMODULE_HPP

#include <type_traits>

#include "string/logger.hpp"
#include "network/connection.hpp"
#include "network/async_service.hpp"
#include "handler/handler.hpp"

/**
 * \brief Module descriptor
 *
 * This class is used to add a module description and to register
 * module-specific classes to their factory.
 */
struct Melanomodule
{
    std::string name;
    std::string description;

    /**
    * \brief Registers a Connection class to ConnectionFactory
    * \param name          Connection protocol name
    */
    template<class ConnectionT>
        void register_connection(const std::string& name)
        {
            static_assert(std::is_base_of<network::Connection,ConnectionT>::value,
                        "Expected network::Connection class");
            network::ConnectionFactory::instance()
                .register_connection(name, &ConnectionT::create);
        }

    /**
    * \brief Registers a log type
    * \param name  Log type name
    * \param color Color used for the log type
    */
    inline void register_log_type(const std::string& name, color::Color12 color)
    {
        Logger::instance().register_log_type(name,color);
    }

    /**
    * \brief Registers a formatter
    * \tparam FormatterT        Formatter class
    * \tparam Args              Constructor parameters
    */
    template <class FormatterT, class... Args>
        void register_formatter(Args&&... args)
        {
            static_assert(std::is_base_of<string::Formatter,FormatterT>::value,
                          "Expected string::Formatter type");
            string::Formatter::registry()
                .add_formatter(new FormatterT(std::forward<Args>(args)...));
        }


    /**
     * \brief Registers a service to ServiceRegistry
     * \tparam ServiceT Name of the service class (must be a singleton)
     * \param name      Service identifier
     */
    template<class ServiceT>
        void register_service(const std::string& name)
        {
            static_assert(std::is_base_of<network::AsyncService,ServiceT>::value,
                          "Expected network::AsyncService type");
            network::ServiceRegistry::instance()
                .register_service(name,&ServiceT::instance());
        }



    /**
    * \brief Registers a Handler to the HandlerFactory
    * \tparam HandlerT  Handler to be registered
    * \param  name      Name to be used in the configuration
    */
    template<class HandlerT>
        void register_handler(const std::string& name)
        {
            static_assert(std::is_base_of<handler::Handler,HandlerT>::value,
                          "Expected handler::Handler type");
            handler::HandlerFactory::instance().register_handler( name,
                [] ( const Settings& settings, MessageConsumer* parent )
                        -> std::unique_ptr<handler::Handler> {
                    return New<HandlerT>(settings,parent);
            });
        }
};

#endif // MELANOMODULE_HPP
