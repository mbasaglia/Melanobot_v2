Melanobot v2
============

A not so simple IRC bot.
For more detailed information on the bot and how to set up an instance,
see https://wiki.evil-ant-colony.org/doku.php?id=melanobot:melanobot

Contacts
--------
Mattia Basaglia <mattia.basaglia@gmail.com>


License
-------
AGPLv3 or later, see COPYING.


Sources
-------

Up to date sources are available at https://github.com/mbasaglia/Melanobot_v2

Installing
==========

Dependencies
------------

* [C++14 Compiler](http://en.cppreference.com/w/cpp/compiler_support)
* [CMake](http://www.cmake.org/)
* [Boost](http://www.boost.org/)
    * asio
    * filesystem
    * program_options
    * property_tree
* [cURLpp](http://jpbarrette.github.io/curlpp/) (MODULE_WEB)
* [OpenSSL](http://openssl.org/) (MODULE_XONOTIC)

Building
--------

    mkdir build && cd build && cmake .. && make

Running
-------

    bin/melanobot

Configuration
-------------

The bot by default tries to load a file called config.info.
This file can is searched in the run directory,
the executable directory, ~/.config/melanobot, or ~/.melanobot.

See configuration examples in ./examples

Installation
------------

TODO
