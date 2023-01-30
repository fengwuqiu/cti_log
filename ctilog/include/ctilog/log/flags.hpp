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
#include <stdint.h>
#include <initializer_list>
namespace cti {
namespace log
{
/**
 * @struct Flag
 * for Flags
 * @sa
 * - Flags
 * - /usr/include/x86_64-linux-gnu/qt5/QtCore/qflags.h
 * - QFlag
 */
struct Flag {
    constexpr inline Flag(int const i) noexcept: i(i) {}
    constexpr inline Flag(unsigned const i) noexcept: i(long(i)) {}
    constexpr inline Flag(long const i) noexcept: i(i) {}
    constexpr inline Flag(long long const i) noexcept: i(i) {}
    constexpr inline Flag(unsigned long const i) noexcept: i(long(i)) {}
    constexpr inline Flag(short const i) noexcept: i(long(i)) {}
    constexpr inline Flag(unsigned short const i) noexcept: i(long(i)) {}
    constexpr inline operator int() const noexcept {
        return int(this->i);
    }
    constexpr inline operator long() const noexcept {
        return this->i;
    }
    constexpr inline operator long long() const noexcept {
        return this->i;
    }
    constexpr inline operator unsigned() const noexcept {
        return static_cast<unsigned>(static_cast<unsigned long>(i));
    }
    constexpr inline operator unsigned long() const noexcept {
        return static_cast<unsigned long>(i);
    }
    constexpr inline operator unsigned long long() const noexcept {
        return static_cast<unsigned long long>(i);
    }
private:
    int64_t i;
};
/**
 * @tparam E enum type
 * @note compiler-generated copy/move ctor/assignment operators are fine!
 * @sa
 * - /usr/include/x86_64-linux-gnu/qt5/QtCore/qflags.h
 * - QFlags
 */
template<typename E>
struct Flags {
    constexpr inline Flags() noexcept: i(0) {}
    constexpr inline Flags(E const& e) noexcept: i(int64_t(e)) {}
    constexpr inline Flags(Flag const& f) noexcept: i(f) {}
    constexpr inline Flags(std::initializer_list<E> flags) noexcept:
        i(Flags::initializerListHelper(flags.begin(), flags.end())) {}
    inline Flags& operator&=(int64_t const mask) noexcept {
        this->i &= mask;
        return *this;
    }
    inline Flags& operator&=(uint64_t const mask) noexcept {
        this->i &= mask;
        return *this;
    }
    inline Flags& operator&=(E const& mask) noexcept {
        i &= int64_t(mask);
        return *this;
    }
    inline Flags& operator|=(Flags const& f) noexcept {
        this->i |= f.i;
        return *this;
    }
    inline Flags& operator|=(E const& f) noexcept {
        this->i |= int64_t(f);
        return *this;
    }
    inline Flags& operator^=(Flags const& f) noexcept {
        this->i ^= f.i;
        return *this;
    }
    inline Flags& operator^=(E const& f) noexcept {
        this->i ^= int64_t(f);
        return *this;
    }
    constexpr inline operator int8_t() const noexcept {
        return int8_t(this->i);
    }
    constexpr inline operator uint8_t() const noexcept {
        return uint8_t(this->i);
    }
    constexpr inline operator int16_t() const noexcept {
        return int16_t(this->i);
    }
    constexpr inline operator uint16_t() const noexcept {
        return uint16_t(this->i);
    }
    constexpr inline operator int32_t() const noexcept {
        return int32_t(this->i);
    }
    constexpr inline operator uint32_t() const noexcept {
        return uint32_t(this->i);
    }
    constexpr inline operator int64_t() const noexcept {
        return this->i;
    }
    constexpr inline operator uint64_t() const noexcept {
        return this->i;
    }
    constexpr inline Flags operator|(Flags const& f) const noexcept {
        return Flags(Flag(this->i | f.i));
    }
    constexpr inline Flags operator|(E const& f) const noexcept {
        return Flags(Flag(this->i | int64_t(f)));
    }
    constexpr inline Flags operator^(Flags const& f) const noexcept {
        return Flags(Flag(this->i ^ f.i));
    }
    constexpr inline Flags operator^(E f) const noexcept {
        return Flags(Flag(this->i ^ int64_t(f)));
    }
    constexpr inline Flags operator&(int64_t const mask) const noexcept {
        return Flags(Flag(this->i & mask));
    }
    constexpr inline Flags operator&(uint64_t const mask) const noexcept {
        return Flags(Flag(this->i & mask));
    }
    constexpr inline Flags operator&(E const& f) const noexcept {
        return Flags(Flag(this->i & int64_t(f)));
    }
    constexpr inline Flags operator~() const noexcept {
        return Flags(Flag(~this->i));
    }
    constexpr inline bool operator!() const noexcept {
        return !this->i;
    }
    inline bool testFlag(E const& f) const noexcept;
protected:
    static_assert((sizeof(E) <= sizeof(int64_t)),
        "Flags uses an int64_t as storage, so an enum with underlying "
        "> int64_t will overflow.");
    constexpr static inline int64_t initializerListHelper(
        typename std::initializer_list<E>::const_iterator it,
        typename std::initializer_list<E>::const_iterator end) noexcept {
        return (it == end ? 0 : (int64_t(*it) | Flags::initializerListHelper(it + 1, end)));
    }
    int64_t i;
};
template<typename E>
inline bool Flags<E>::testFlag(E const& f) const noexcept
{
    int64_t const v(static_cast<int64_t>(f));
    return ((this->i & v) == v) && ((v != 0) || (this->i == v));
}
}//namespace log
}//namespace cti
