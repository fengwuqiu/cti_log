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
#include <string>
namespace cti {
namespace log
{
enum class Errno: int {
    Ok = 0, /* success */
    NotSet, /* optional or related not set */
    InvalidType, /* invalid type */
    BadAnyCast, /* any cast fail */
    AlreadyOpen, /* file already open */
    InvalidFilename, /* invalid filename */
    InvalidData, /* invalid data */
    InvalidParam, /* invalid param */
    OpenError, /* open error */
    StatError, /* get stat error */
    NotOpen, /* not open yet */
    NoMem, /* no memory */
    ReadError, /* read error */
    Cancelled, /* cancelled */
    Busy, /* busy */
    Interrupt, /* interrupt requested */
    SeekError, /* seek error */
    FileChanged, /* file changed */
};
static constexpr inline int GetErrno(Errno const& e) noexcept
{
    return static_cast<int>(e);
}
constexpr int kCppbaseErrnoOffset = 100000;
static constexpr inline int GetErrno(int const e) noexcept
{
    return static_cast<int>(e) + kCppbaseErrnoOffset;
}
extern std::string ErrnoToString(Errno const& e) noexcept;
static inline std::string ErrnoToString(int const e) noexcept
{
    return ErrnoToString(static_cast<Errno>(e));
}
}//namespace log
}//namespace cti
