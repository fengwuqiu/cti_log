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
#include <stdexcept>
namespace cti {
namespace log
{
/**
 * @struct Exception
 * std::exception + code and trace
 */
struct Exception: public std::exception {
    /// default ctor
    Exception() noexcept;
    /**
     * @brief ctor
     * @param msg basic runtime_error msg
     * @param code exception code
     * @param file exception in this file
     * @param line exception at this line
     */
    Exception(
        std::string const& msg,
        int const code,
        char const* file,
        int const line) noexcept;
    /**
     * @brief copy ctor
     * @param other copy source
     */
    Exception(Exception const& other) noexcept;
    /**
     * @brief move ctor
     * @param other move source
     */
    Exception(Exception&& other) noexcept;
    /// dtor
    virtual ~Exception() noexcept;
    /**
     * @brief assign
     * @param other assign source
     */
    Exception& operator=(Exception const& other) noexcept;
    /// @return exception message
    inline std::string const& getMsg() const noexcept { return this->msg; }
    /// @return exception code
    inline int getCode() const noexcept { return this->code; }
    /**
     * @return a C-style character string describing the general cause of
     * the current error (the same string passed to the ctor).
     */
    virtual char const* what() const noexcept override;
protected:
    std::string msg;///< excpetion message
    int code;///< exception code
};
}//namespace log
}//namespace cti
