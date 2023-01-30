/* This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/**
 * @file ctilog/loghelper.cpp.hpp
 * @note Use in CPP file !!
 *
 * - 1 include ctilog/log.hpp
 * - 2 include this file
 * - 3 define a const char* constexpr or string named kN
 */
#pragma once
#ifndef CTI_BASE_LOG_HPP
    #error("Please include ctilog/log.hpp before this file")
#endif
/// @def Assert assert, throw a exception when fail
#define Assert(cond, msg) if (!(cond)) { \
    std::string e = std::string(#cond " fail: "); \
    e += msg; \
    e += " (" __FILE__ "+"; \
    e += std::to_string(__LINE__) + ")"; \
    cti::log::Logger::getLogger().e(kN, e); \
    throw std::runtime_error(e); \
}
/// @def Throw
#define Throw(msg) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().e(kN, ss.str()); \
    throw std::runtime_error(ss.str()); \
}
/// @def Fatal output fatal log
#define Fatal(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Fata)) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().f(kN, ss.str()); \
}
/// @def Error output error log
#define Error(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Erro)) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().e(kN, ss.str()); \
}
/// @def Warning output warning log
#define Warn(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Warn)) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().w(kN, ss.str()); \
}
/// @def Note output note log
#define Note(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Note)) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().n(kN, ss.str()); \
}
/// @def Info output info log
#define Info(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Info)) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().i(kN, ss.str());\
}
/// @def Trace output trace log
#define Trace() cti::log::Logger::getLogger().append(\
    kN, __FILE__, __LINE__, __func__, cti::log::LogLevel::Trac)
/// @def Debug output debug log
#define Debug(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Debu)) { \
    std::stringstream ss; \
    ss << msg << " (" << __FILE__ << "+" << __LINE__ << ")"; \
    cti::log::Logger::getLogger().d(kN, ss.str()); \
}
/// @def Detail output debug log
#define Detail(msg) if (cti::log::Logger::getLogger().isLogable(cti::log::LogLevel::Deta)) { \
    std::stringstream ss; \
    ss << msg; \
    cti::log::Logger::getLogger().append(kN, __FILE__, __LINE__, ss.str(), cti::log::LogLevel::Deta); \
}

