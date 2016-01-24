/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef MULTIBUF_HPP
#define MULTIBUF_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "maybe_ptr.hpp"

/**
 * \brief (Output) Stream buffer supporting multiple targets (defined by other buffers)
 */
class Multibuf : public std::streambuf
{
public:
    Multibuf() {}
    Multibuf(const Multibuf&) = delete;
    Multibuf(Multibuf&&) = delete;
    Multibuf& operator=(const Multibuf&) = delete;
    Multibuf& operator=(Multibuf&&) = delete;
    ~Multibuf() = default;

    /**
     * \brief Adds a buffer to the list of targets
     * \param buffer         The buffer to add
     * \param take_ownership Whether this object shall handle the destruction of \c buffer
     */
    void push_buffer(std::streambuf* buffer, bool take_ownership = false)
    {
        buffers.emplace_back(buffer,take_ownership);
    }

    /**
     * \brief Opens a file buffer and adds it to the list of targets
     * \returns Whether the file has been open successfully
     */
    bool push_file(const std::string& name, std::ios::openmode mode = std::ios::out|std::ios::app)
    {
        auto buf = MaybePtr<std::filebuf>::make();
        buf->open(name, mode);
        if ( buf->is_open() )
        {
            buffers.push_back(std::move(buf));
            return true;
        }
        return false;
    }

protected:
    /**
     * \brief Writes \c c to the target buffers
     */
    int overflow(int c) override
    {
        for ( auto& item : buffers )
            item->sputc(c);
        return 0;
    }

    /**
     * \brief Calls \c sync on the target buffers
     */
    int sync() override
    {
        for ( auto& item : buffers )
            item->pubsync();
        return 0;
    }

private:

    std::vector<MaybePtr<std::streambuf>> buffers;
};

#endif // MULTIBUF_HPP
