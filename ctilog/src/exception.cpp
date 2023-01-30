/* ========================================================================
 * This library is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ======================================================================== */
#include "ctilog/log/exception.hpp"
#include <sstream>
namespace cti {
namespace log
{
/// default ctor
Exception::Exception() noexcept:
    Exception("", 0, "", 0) {}
/**
 * @brief ctor
 * @param msg basic runtime_error msg
 * @param code exception code
 * @param file exception in this file
 * @param line exception at this line
 */
Exception::Exception(
    std::string const& msg,
    int const code,
    char const* file,
    int const line) noexcept: std::exception(), code(code)
{
    std::stringstream ss;
    ss << "Exception: msg: `" << msg << "', code: `" << code
        << "', file: `" << file << "', line: `" << line << "'.";
    this->msg = ss.str();
}
/**
 * @brief copy ctor
 * @param other copy source
 */
Exception::Exception(Exception const& other) noexcept:
    std::exception(), msg(other.msg), code(other.code) {}
/**
 * @brief move ctor
 * @param other move source
 */
Exception::Exception(Exception&& other) noexcept: Exception()
{
    std::swap<std::exception>(*this, other);
    this->msg = std::move(other.msg);
    this->code = other.code;
    other.code = 0;
}
/// dtor
Exception::~Exception() noexcept {}
/**
 * @brief assign
 * @param other assign source
 */
Exception& Exception::operator=(Exception const& other) noexcept
{
    *(static_cast<std::exception*>(this)) = other;
    this->msg = other.msg;
    this->code = other.code;
    return *this;
}
/**
 * @return a C-style character string describing the general cause of
 * the current error (the same string passed to the ctor).
 */
const char* Exception::what() const noexcept
{
    return this->msg.c_str();
}
}//namespace log
}//namespace cti
