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
#pragma once
namespace cti {
namespace log
{
enum class LogLevel: uint32_t {
    Min  = 0,
    Fata = Min,
    Erro,
    Warn,
    Note,
    Info,
    Trac,
    Debu,
    Deta,
    Max = Deta,
    Unchange = Max + 2,
};
/// Global LogLevel for debug
extern LogLevel kLogLevel;
/// LogLevel to string
extern std::string logLevelToString(LogLevel const& logLevel) noexcept;
static inline std::ostream& operator<<(std::ostream& os, LogLevel const& logLevel) noexcept
{
    return os << logLevelToString(logLevel);
}
}//namespace log
}//namespace cti

