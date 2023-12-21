/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2023 Miroslav Hutar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <gtkmm.h>

enum Level
{
    debug,
    error,
    warning,
    info
};

class Logger
{
    std::ostream *stream; // set this in a constructor to point
                          // either to a file or console stream

public:
    inline static Level debugLevel = info;
    inline static Glib::RefPtr<Gtk::TextBuffer> guiOutput;

    inline static Logger *INSTANCE = nullptr;

    static Logger &log(Level n, bool persist = false)
    {
        if (INSTANCE && !persist)
        {
            delete INSTANCE;
            INSTANCE = nullptr;
        }
        
        if (!INSTANCE)
        {
            INSTANCE = new Logger();

            auto now = std::chrono::system_clock::now();
            auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
            auto timer = std::chrono::system_clock::to_time_t(now);
            std::tm bt = *std::localtime(&timer);
            std::ostringstream oss;
            oss << "[";
            oss << std::put_time(&bt, "%H:%M:%S"); // HH:MM:SS
            oss << '.' << std::setfill('0') << std::setw(6) << mcs.count();
            oss << "] ";
            *INSTANCE << oss.str().c_str();
        }
        Logger::debugLevel = n;
        return *INSTANCE;
    }

    static gboolean guiLog(void *data)
    {
        std::string *msg = (std::string *)data;
        Logger::guiOutput->insert_at_cursor(/* Logger::guiOutput->get_iter_at_offset(-1), */ *msg);
        delete msg;
        return G_SOURCE_REMOVE;
    }

    template <class T>
    Logger &operator<<(const T &v)
    {
        if (Logger::guiOutput)
        {
            std::stringstream ss;
            ss << v;
            std::cout << ss.str();
            std::string *d = new std::string(ss.str());
            gdk_threads_add_idle(guiLog, (void*)d);
        } else {
            std::cout << v;
        }
        
        return *this;
    }
};

#endif