/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include "melanolib/dynlib/library.hpp"
#include "melanolib/utils/c++-compat.hpp"
#include "melanobot/storage.hpp"
#include "melanobot/config_factory.hpp"
#include "melanobot/melanobot.hpp"

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
    melanolib::Optional<melanolib::dynlib::Library> library; ///< Set at runtime
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
 * \brief Registers a global service to ServiceRegistry
 * \note This is intended to be used for singleton service classes
 * \tparam ServiceT Name of the service class (must be a singleton)
 * \param name      Service identifier
 */
template<class ServiceT>
    void register_service(const std::string& name)
{
    static_assert(std::is_base_of<AsyncService,ServiceT>::value,
                    "Expected AsyncService type");
    ServiceRegistry::instance()
        .register_service(name,&ServiceT::instance());
}

/**
 * \brief Registers an instantiable service
 * \note This is intended to be used for service classes for which you can create objects
 * \tparam ServiceT Name of the service class
 * \param name      Service identifier
 */
template<class ServiceT>
    void register_instantiable_service(const std::string& name)
{
    static_assert(std::is_base_of<AsyncService, ServiceT>::value,
                    "Expected AsyncService type");

    melanobot::ConfigFactory::instance().register_item(name,
        [](const std::string&, const Settings& settings, MessageConsumer*)
        {
            auto service = melanolib::New<ServiceT>();
            service->initialize(settings);
            melanobot::Melanobot::instance().add_service(std::move(service));
            return true;
        }
    );
}

/**
 * \brief Registers a Handler to the ConfigFactory
 * \tparam HandlerT  Handler to be registered
 * \param  name      Name to be used in the configuration
 */
template<class HandlerT>
    void register_handler(const std::string& name)
{
    static_assert(std::is_base_of<melanobot::Handler, HandlerT>::value,
                    "Expected melanobot::Handler type");

    melanobot::ConfigFactory::instance().register_item( name,
        [](const std::string&  handler_name, const Settings& settings, MessageConsumer* parent)
        {
            parent->add_handler(melanolib::New<HandlerT>(settings, parent));
            return true;
        }
    );
}

/**
 * \brief Registers a file storage back-end
 * \tparam StorageT  Storage to be registered
 * \param  name      Name to be used in the configuration
 */
template<class StorageT>
    void register_storage(const std::string& name)
    {
        static_assert(std::is_base_of<melanobot::StorageBase, StorageT>::value,
                        "Expected melanobot::StorageBase type");
        melanobot::StorageFactory::instance().register_type( name,
            [] ( const Settings& settings ) {
                return melanolib::New<StorageT>(settings);
        });
    }

} // namespace module

#define MELANOMODULE_ENTRY_POINT extern "C"

#endif // MELANOMODULE_HPP
