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
#include "melanolib/library.hpp"
#include "melanolib/c++-compat.hpp"
#include "storage.hpp"

namespace module {

/**
 * \brief Module descriptor
 *
 * This class is used to add a module description and to register
 * module-specific classes to their factory.
 */
struct Melanomodule
{

    /**
     * \brief Module dependency descriptor
     */
    struct Dependency
    {
        /**
         * \brief Name of the module
         */
        std::string module;

        /**
         * \brief Minimum version.
         *
         * Zero means no minimum version
         */
        int minimum_version = 0;

        /**
         * \brief Maximum version.
         *
         * Zero means no maximum version
         */
        int maximum_version = 0;

        Dependency(std::string module, int minimum_version = 0, int maximum_version = 0)
            : module(std::move(module)),
            minimum_version(minimum_version),
            maximum_version(maximum_version)
        {}

        /**
         * \brief Whether a plugin matches this dependency
         */
        bool match(const Melanomodule& module) const
        {
            if ( module.name != this->module )
                return false;
            if ( minimum_version && minimum_version > module.version )
                return false;
            if ( maximum_version && maximum_version < module.version )
                return false;
            return true;
        }

        bool satisfied(const std::vector<Melanomodule>& modules) const
        {
            return std::any_of(modules.begin(), modules.end(),
                [this](const auto& module) { return this->match(module); } );
        }
    };

    Melanomodule(
        std::string name,
        std::string description = {},
        int version = 0,
        std::vector<Dependency> dependencies = {}
    ) : name(std::move(name)),
        description(std::move(description)),
        version(version),
        dependencies(std::move(dependencies))
    {}

    bool dependencies_satisfied(const std::vector<Melanomodule>& modules) const
    {
        return std::all_of(dependencies.begin(), dependencies.end(),
            [&modules](const auto& dep) { return dep.satisfied(modules); } );
    }

    bool is_deprecated(const std::vector<Melanomodule>& modules) const
    {
        return std::any_of(modules.begin(), modules.end(),
            [this](const auto& mod){
                return mod.name == name && mod.version > version;
        });
    }

    /**
     * \brief Cpmparator that groups redundant modules together
     *
     * When used to sort a sequence of modules, those with the same name
     * will be grouped together, and those groups will be sorted by
     * descending version.
     *
     * It is not operator< because an ordering relation doesn't make much sense.
     */
    static bool lexcompare(const Melanomodule& a, const Melanomodule& b)
    {
        auto cmp = a.name.compare(b.name);
        return cmp < 0 || ( cmp == 0 && a.version > b.version );
    }

    std::string name;
    std::string description;
    int         version = 0;
    std::vector<Dependency> dependencies;

    /// \todo Make this an implementation detail if possible
    melanolib::Optional<melanolib::library::Library> library; ///< Set at runtime
};

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
        static_assert(std::is_base_of<handler::Handler, HandlerT>::value,
                        "Expected handler::Handler type");
        handler::HandlerFactory::instance().register_handler( name,
            [] ( const Settings& settings, MessageConsumer* parent )
                    -> std::unique_ptr<handler::Handler> {
                return melanolib::New<HandlerT>(settings,parent);
        });
    }

/**
 * \brief Registers a file storage back-end
 * \tparam StorageT  Storage to be registered
 * \param  name      Name to be used in the configuration
 */
template<class StorageT>
    void register_storage(const std::string& name)
    {
        static_assert(std::is_base_of<storage::StorageBase, StorageT>::value,
                        "Expected storage::StorageBase type");
        storage::StorageFactory::instance().register_type( name,
            [] ( const Settings& settings ) {
                return melanolib::New<StorageT>(settings);
        });
    }

} // namespace module

#define MELANOMODULE_ENTRY_POINT extern "C"

#endif // MELANOMODULE_HPP
