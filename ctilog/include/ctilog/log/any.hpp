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
#include <typeinfo>
#include <functional>
#include "boost/any.hpp"
#include "boost/optional.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/thread/shared_mutex.hpp"
#include "ctilog/log/anytype.hpp"
#include "ctilog/log/errno.hpp"
#include <iostream>

namespace cti {
namespace log
{
typedef boost::shared_ptr<void> AnyDataType;
typedef boost::shared_ptr<void const> AnyDataConstType;
typedef std::vector<uint8_t> AnyVector;
typedef std::vector<uint8_t> const AnyConstVector;
typedef boost::shared_ptr<AnyVector> AnyVectorPtr;
typedef boost::shared_ptr<AnyConstVector> AnyVectorConstPtr;
struct Any {
    Any();
    /* @note @a type should be valid and cannot be AnyType::Void
    Any(boost::any&& data, AnyType const& type, bool const);*/
    template<typename T>
    Any& init(boost::any const& value) {
        this->initByPtr<T>(value);
        if (!(*this)) {
            this->initByObj<T>(value);
        }
        return *this;
    }
    /**
     * @brief init from ptr or obj
     * NOTE is obj is Anyable please use fromAnyable
     * @param value the any
     * @param isPtr is true is shared_ptr else false
     * @throw bad_any_cast when cast fail
     */
    template<typename T>
    Any& init(boost::any const& value, bool const isPtr) {
        if (isPtr) {
            this->initByPtr<T>(value);
        } else {
            this->initByObj<T>(value);
        }
        return *this;
    }
    /*Any(Any&& other);*//* TODO move ctor */
    Any(Any const& other);
    template<typename T>
    int __toTPtr(
        boost::any const& value,
        boost::typeindex::stl_type_index const& typeinfo) {
        /*boost::typeindex::stl_type_index parsedtypeinfo = GetTypeInfo<
            boost::shared_ptr<T> >();*/
        boost::typeindex::stl_type_index parsedtypeinfoc = GetTypeInfo<
            boost::shared_ptr<T const> >();
        if (typeinfo != parsedtypeinfoc) {
            return GetErrno(Errno::InvalidType);
        }
        int ret;
        std::tie(ret, this->data) = Any::toTPtr<T>(value);
        if (ret < 0) {
            return ret;
        } else {
            this->typeinfo = GetTypeInfo<T>();
            return 0;
        }
    }
    template<typename T>
    Any& initByPtr(boost::any const& value) {
        boost::typeindex::stl_type_index const typeinfo = GetTypeInfo<
            boost::shared_ptr<T const> >();
        boost::upgrade_lock<boost::shared_mutex> uplock(this->rwlock);
        boost::upgrade_to_unique_lock<boost::shared_mutex> wLock(uplock);
        int ret = this->__toTPtr<bool>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<char>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<short>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<int>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<long>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<long long>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<unsigned char>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<unsigned short>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<unsigned>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<unsigned long>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<unsigned long long>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<std::string>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<AnyVector>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<DataAndSize>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        ret = this->__toTPtr<Anyable>(value, typeinfo);
        if (ret < 0) {
            goto fail;
        } else if (0 == ret) {
            return *this;
        }
        if (value.type() != typeinfo) {
            goto fail;
        } else {
            this->typeinfo = GetTypeInfo<T>();
            this->data = boost::any_cast<boost::shared_ptr<T const> >(value);
            return *this;
        }
    fail:
        std::cerr << "cast fail: expect type `" << typeinfo.name()
            << "' type name `" <<  value.type().name() << "'\n";
        this->data = nullptr;
        this->typeinfo = GetTypeInfo<void>();
        return *this;
    }
    template<typename T>
    Any& initByObj(boost::any const& value) {
        int ret;
        boost::typeindex::stl_type_index parsedtypeinfo;
        boost::typeindex::stl_type_index const typeinfo = GetTypeInfo<T>();
        boost::upgrade_lock<boost::shared_mutex> uplock(this->rwlock);
        boost::upgrade_to_unique_lock<boost::shared_mutex> wLock(uplock);
        (void)(wLock);
        parsedtypeinfo = GetTypeInfo<bool>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<bool> v;
            std::tie(ret, v) = Any::toT<bool>(value);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<bool>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<char>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<char> v;
            std::tie(ret, v) = Any::toT<char, bool>(
                value, std::placeholders::_1);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<char>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<short>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<short> v;
            std::tie(ret, v) = Any::toT<short, char, bool>(
                value,
                std::placeholders::_1,
                std::placeholders::_2);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<short>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<int>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<int> v;
            std::tie(ret, v) = Any::toT<int, short, char, bool>(
                value,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<int>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<long>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<long> v;
            std::tie(ret, v) = Any::toT<long, int, short, char, bool>(
                value,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<long>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<long long>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<long long> v;
            /*std::tie(ret, v) = Any::toLongLong(value);*/
            std::tie(ret, v) = Any::toT<
                long long, long, int, short, char, bool>(
                value,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4,
                std::placeholders::_5);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<long long>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<unsigned char>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<unsigned char> v;
            std::tie(ret, v) = Any::toT<unsigned char, bool>(
                value, std::placeholders::_1);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<unsigned char>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<unsigned short>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<unsigned short> v;
            std::tie(ret, v) = Any::toT<unsigned short, unsigned char, bool>(
                value,
                std::placeholders::_1,
                std::placeholders::_2);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<unsigned short>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<unsigned int>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<unsigned int> v;
            std::tie(ret, v) = Any::toT<
                unsigned,
                unsigned short,
                unsigned char,
                bool>(
                    value,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<unsigned>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<unsigned long>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<unsigned long> v;
            std::tie(ret, v) = Any::toT<
                unsigned long,
                unsigned,
                unsigned short,
                unsigned char,
                bool>(
                    value,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    std::placeholders::_4);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<unsigned long>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<unsigned long long>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<unsigned long long> v;
            std::tie(ret, v) = Any::toT<
                unsigned long long,
                unsigned long,
                unsigned,
                unsigned short,
                unsigned char,
                bool>(
                    value,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    std::placeholders::_4,
                    std::placeholders::_5);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<unsigned long long>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<std::string>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<std::string> v;
            /*std::tie(ret, v) = Any::toString(value);*/
            std::tie(ret, v) = Any::toT<
                std::string,
                char const*,
                char *>(value, std::placeholders::_1, std::placeholders::_2);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<std::string>(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<AnyVector>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<AnyVector> v;
            /*std::tie(ret, v) = Any::toVector(value);*/
            std::tie(ret, v) = Any::toT<AnyVector>(value);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<std::vector<uint8_t> >(*v);
            goto success;
        }
        parsedtypeinfo = GetTypeInfo<DataAndSize>();
        if (typeinfo == parsedtypeinfo) {
            boost::optional<DataAndSize> v;
            std::tie(ret, v) = Any::toT<DataAndSize>(value);
            if (!v) {
                goto fail;
            }
            this->data = boost::make_shared<DataAndSize>(*v);
            goto success;
        }
        goto fail;
    success:
        this->typeinfo = parsedtypeinfo;
        return *this;
    fail:
        std::cerr << "cast fail expect type `" << typeinfo.name()
            << "' type name `" << value.type().name() << "'\n";
        this->data = nullptr;
        this->typeinfo = GetTypeInfo<void>();
        return *this;
    }
    virtual ~Any();
    /*Any assign(boost::any&& value, AnyType const& type);*/
    template <typename T>
    inline Any& assign(boost::any const& value) {
        this->init<T>(value);
        return *this;
    }
    template <typename T>
    inline Any& assign(boost::any const& value, bool const isPtr) {
        this->init<T>(value, isPtr);
        return *this;
    }
    /*Any& operator=(Any&& other);*/
    Any& operator=(Any const& other);
    bool operator==(Any const& other) const;
    /// Is set or not
    inline operator bool() const {
        if (this->data) {
            return true;
        } else {
            return false;
        }
    }
    /// @return true when is binary else return false when text
    bool isBinary() const;
    boost::typeindex::stl_type_index getTypeInfo() const {
        return this->typeinfo;
    }
    /** @return {ret,ptr,size} / {ret,ptr,size,isBinary} */
    std::tuple<int, void const*, size_t> getDataBundle(
        bool* const isBinary = nullptr) const;
    std::tuple<int, void const*, size_t, bool> getDataBundleb() const;
    /** @brief reset to not set */
    void reset();
    /** @brief to T const ptr */
    template <typename T>
    std::tuple<int, boost::shared_ptr<T const> > toTPtr() const {
        boost::shared_lock<boost::shared_mutex> rLock(this->rwlock);
        (void)(rLock);
        if (!this->data) {
            return std::tuple<int, boost::shared_ptr<T const> >(
                -GetErrno(Errno::NotSet),
                nullptr);
        }
        if ((GetTypeInfo<void>() == this->typeinfo) ||
            ((typeid(T) != this->typeinfo) &&
            (typeid(T const) != this->typeinfo))) {
            return std::tuple<int, boost::shared_ptr<T const> >(
                -GetErrno(Errno::InvalidType),
                nullptr);
        }
        return Any::toTPtr<T>(this->data);
    }
    template <typename T>
    boost::shared_ptr<T const> toTPtrd(
        boost::shared_ptr<T const> const& dft,
        int* const e = nullptr) const {
        boost::shared_lock<boost::shared_mutex> rLock(this->rwlock);
        (void)(rLock);
        if (!this->data) {
            if (e) {
                *e = -GetErrno(Errno::NotSet);
            }
            return dft;
        }
        if ((GetTypeInfo<void>() == this->typeinfo) ||
            ((typeid(T) != this->typeinfo) &&
            (typeid(T const) != this->typeinfo))) {
            if (e) {
                *e = -GetErrno(Errno::InvalidType);
            }
            return nullptr;
        }
        int ret;
        boost::shared_ptr<T const> ptr;
        std::tie(ret, ptr) = Any::toTPtr<T>(this->data);
        if (e) {
            *e = ret;
        }
        return ptr;
    }
    /**
     * @brief cast self to T
     * @note T should not be wrapper ptr type else use toTPtr
     */
    template <typename T>
    std::tuple<int, boost::optional<T> > toT() const {
        boost::shared_lock<boost::shared_mutex> rLock(this->rwlock);
        (void)(rLock);
        boost::optional<T> const nilopt;
        if (!this->data) {
            return std::tuple<int, boost::optional<T> >(
                -GetErrno(Errno::NotSet),
                nilopt);
        }
        if ((GetTypeInfo<void>() == this->typeinfo) ||
            ((typeid(T) != this->typeinfo) &&
            (typeid(T const) != this->typeinfo))) {
            return std::tuple<int, boost::optional<T> >(
                -GetErrno(Errno::InvalidType),
                nilopt);
        }
        boost::shared_ptr<T const> ptr = Any::reinterpretToTPtr<T>(this->data);
        if (!ptr) {
            return std::tuple<int, boost::optional<T> >(
                -GetErrno(Errno::BadAnyCast),
                nilopt);
        }
        return std::tuple<int, boost::optional<T> >(
            GetErrno(Errno::Ok),
            *ptr);
    }
    template <typename T>
    T toT(T const& dft, int* const e = nullptr) const {
        boost::shared_lock<boost::shared_mutex> rLock(this->rwlock);
        (void)(rLock);
        boost::optional<T> const nilopt;
        if (!this->data) {
            if (e) {
                *e = -GetErrno(Errno::NotSet);
            }
            return dft;
        }
        if ((GetTypeInfo<void>() == this->typeinfo) ||
            ((typeid(T) != this->typeinfo) &&
            (typeid(T const) != this->typeinfo))) {
            if (e) {
                *e = -GetErrno(Errno::InvalidType);
            }
            return dft;
        }
        boost::shared_ptr<T const> ptr = Any::reinterpretToTPtr<T>(this->data);
        if (!ptr) {
            if (e) {
                *e = -GetErrno(Errno::BadAnyCast);
            }
            return dft;
        }
        if (e) {
            *e = GetErrno(Errno::Ok);
        }
        return *ptr;
    }
    /** @brief check if this is type T */
    template <typename T>
    inline bool isT() const {
        return typeid(T) == this->typeinfo;
    }
    /**
     * @param data
     * - for LongLong: data is numeric c string size will be ignore
     * - for String: data is c string size will be ignore
     * @param size the size is the data size
     * (e.x. for String: strlen(data) + 1)
     * @param typeinfo the type
     * @note data and size should be valid
     */
    static Any create(
        void const* const data,
        size_t const size,
        boost::typeindex::stl_type_index const& typeinfo);
    template <typename T>
    static Any create(void const* const data, size_t const size) {
        AnyDataType value;
        if (Any::isSameT<T, long long>()) {
            value = AnyDataType(new long long(::atoll(
                reinterpret_cast<char const*>(data))));
            return Any().init<long long>(value, true);
        } else if (Any::isSameT<T, std::string>()) {
            value = AnyDataType(new std::string(reinterpret_cast<char const *>(
                data)));
            return Any().init<std::string>(value, true);
        } else if (Any::isSameT<T, AnyVector>()) {
            boost::shared_ptr<std::vector<uint8_t> > const v(
                new std::vector<uint8_t>(size));
            ::memcpy(v->data(), data, size);
            value = v;
            return Any().init<AnyVector>(value, true);
        } else {
            return Any();
        }
    }
    template <typename T>
    static inline Any fromT(T const& value) {
        return Any().init<T>(value, false);
    }
    template <typename T>
    static inline Any fromTPtr(boost::shared_ptr<T const> const& value) {
        return Any().init<T>(value, true);
    }
    /** @brief create Any from bool */
    static inline Any fromBool(bool const value) {
        return Any::fromT<bool>(value);
    }
    /** @brief create Any from short */
    static inline Any fromShort(short const value) {
        return Any::fromT<short>(value);
    }
    /** @brief create Any from int */
    static inline Any fromInt(int const value) {
        return Any::fromT<int>(value);
    }
    /** @brief create Any from long */
    static inline Any fromLong(long const value) {
        return Any::fromT<long>(value);
    }
    /** @brief create Any from long long */
    static inline Any fromLongLong(long long const value) {
        return Any::fromT<long long>(value);
    }
    /** @brief create Any from string */
    static inline Any fromString(std::string const& value) {
        return Any::fromT<std::string>(value);
    }
    /** @brief create Any from AnyConstVector */
    static inline Any fromVector(AnyConstVector& value) {
        return Any::fromT<AnyVector>(value);
    }
    /// Create Any from AnyVectorConstPtr
    static inline Any fromVector(AnyVectorConstPtr const& value) {
        return Any::fromTPtr<AnyVector>(value);
    }
    static inline Any fromAnyable(Anyable const& value) {
        return Any::fromTPtr<Anyable>(value.clone());
    }
    static inline Any fromAnyable(AnyableConstPtr const& value) {
        return Any::fromTPtr<Anyable>(value);
    }
    /// Cast to type T boost const shared_ptr
    template <typename T>
    static inline std::tuple<int, boost::shared_ptr<T const> > toTPtr(
        boost::any const& value) {
        boost::shared_ptr<T const> casted;
        int ret;
        try {
            if (Any::isT<AnyDataConstType>(value)) {
                AnyDataConstType const rawvalue = boost::any_cast<
                    AnyDataConstType>(value);
                casted = Any::reinterpretToTPtr<T>(rawvalue);
            } else if (Any::isT<AnyDataType>(value)) {
                AnyDataType const raw = boost::any_cast<AnyDataType>(value);
                casted = Any::reinterpretToTPtr<T>(raw);
            } else if (Any::isT<boost::shared_ptr<T const> >(value)) {
                casted = boost::any_cast<boost::shared_ptr<T const> >(value);
            } else {
                casted = boost::any_cast<boost::shared_ptr<T> >(value);
            }
            ret = 0;
        } catch (boost::bad_any_cast const&) {
            casted = nullptr;
            ret = -GetErrno(Errno::BadAnyCast);
        }
        return std::tuple<int, boost::shared_ptr<T const> >(ret, casted);
    }
    /// Cast to type T from any and with 1 primary
    template <typename T>
    static std::tuple<int, boost::optional<T> > toT(boost::any const& value) {
        if (Any::isT<T const>(value)) {
            try {
                T const castedvalue = boost::any_cast<T const>(value);
                return std::tuple<int, boost::optional<T> >(0, castedvalue);
            } catch (boost::bad_any_cast const&) {
                boost::optional<T> nilopt;
                return std::tuple<int, boost::optional<T> >(
                    -GetErrno(Errno::BadAnyCast),
                    nilopt);
            }
        } else if (Any::isT<T>(value)) {
            try {
                T const castedvalue = boost::any_cast<T>(value);
                return std::tuple<int, boost::optional<T> >(0, castedvalue);
            } catch (boost::bad_any_cast const&) {
                boost::optional<T> nilopt;
                return std::tuple<int, boost::optional<T> >(
                    -GetErrno(Errno::BadAnyCast),
                    nilopt);
            }
        } else {
            boost::optional<T> nilopt;
            return std::tuple<int, boost::optional<T> >(
                -GetErrno(Errno::BadAnyCast),
                nilopt);
        }
    }
    /// Cast to type T from any and with 1 primary and 1 candidates
    template <typename T, typename C1>
    static std::tuple<int, boost::optional<T> > toT(
        boost::any const& value,
        std::_Placeholder<1> const& /*_1*/) {
        int ret;
        T const init0 = T();
        boost::optional<T> v0(init0);
        std::tie(ret, v0) = Any::toT<T>(value);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v0);
        }
        C1 const init1 = C1();
        boost::optional<C1> v1(init1);
        std::tie(ret, v1) = Any::toT<C1>(value);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(
                ret,
                static_cast<T>(*v1));
        }
        boost::optional<T> nilopt;
        return std::tuple<int, boost::optional<T> >(
            -GetErrno(Errno::BadAnyCast),
            nilopt);
    }
    /// Cast to type T from any and with 1 primary and 2 candidates
    template <typename T, typename C1, typename C2>
    static std::tuple<int, boost::optional<T> > toT(
        boost::any const& value,
        std::_Placeholder<1> const& _1,
        std::_Placeholder<2> const& /*_2*/) {
        int ret;
        T const init = T();
        boost::optional<T> v(init);
        std::tie(ret, v) = Any::toT<T>(value);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C1>(value, _1);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C2>(value, _1);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        boost::optional<T> nilopt;
        return std::tuple<int, boost::optional<T> >(
            -GetErrno(Errno::BadAnyCast),
            nilopt);
    }
    /// Cast to type T from any and with 1 primary and 3 candidates
    template <typename T, typename C1, typename C2, typename C3>
    static std::tuple<int, boost::optional<T> > toT(
        boost::any const& value,
        std::_Placeholder<1> const& _1,
        std::_Placeholder<2> const& _2,
        std::_Placeholder<3> const& /*_3*/) {
        int ret;
        T const init = T();
        boost::optional<T> v(init);
        std::tie(ret, v) = Any::toT<T>(value);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C1, C2>(value, _1, _2);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C3>(value, _1);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        boost::optional<T> nilopt;
        return std::tuple<int, boost::optional<T> >(
            -GetErrno(Errno::BadAnyCast),
            nilopt);
    }
    /// Cast to type T from any and with 1 primary and 4 candidates
    template <typename T, typename C1, typename C2, typename C3, typename C4>
    static std::tuple<int, boost::optional<T> > toT(
        boost::any const& value,
        std::_Placeholder<1> const& _1,
        std::_Placeholder<2> const& _2,
        std::_Placeholder<3> const& _3,
        std::_Placeholder<4> const& /*_4*/) {
        int ret;
        T const init = T();
        boost::optional<T> v(init);
        std::tie(ret, v) = Any::toT<T>(value);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C1, C2, C3>(value, _1, _2, _3);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C4>(value, _1);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        boost::optional<T> nilopt;
        return std::tuple<int, boost::optional<T> >(
            -GetErrno(Errno::BadAnyCast),
            nilopt);
    }
    /// Cast to type T from any and with 1 primary and 5 candidates
    template <
        typename T,
        typename C1, typename C2, typename C3, typename C4, typename C5>
    static std::tuple<int, boost::optional<T> > toT(
        boost::any const& value,
        std::_Placeholder<1> const& _1,
        std::_Placeholder<2> const& _2,
        std::_Placeholder<3> const& _3,
        std::_Placeholder<4> const& _4,
        std::_Placeholder<5> const& /*_5*/) {
        int ret;
        T const init = T();
        boost::optional<T> v(init);
        std::tie(ret, v) = Any::toT<T>(value);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C1, C2, C3, C4>(value, _1, _2, _3, _4);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C1, C2, C3>(value, _1, _2, _3);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        std::tie(ret, v) = Any::toT<T, C4>(value, _1);
        if (0 == ret) {
            return std::tuple<int, boost::optional<T> >(ret, v);
        }
        boost::optional<T> nilopt;
        return std::tuple<int, boost::optional<T> >(
            -GetErrno(Errno::BadAnyCast),
            nilopt);
    }
    /** @brief check any if is type T */
    template <typename T>
    static inline bool isT(boost::any const& value) {
        return value.type() == typeid(T);
    }
    template <typename T>
    static inline bool isSameT(boost::typeindex::stl_type_index const& other) {
        return typeid(T) == other;
    }
    template <typename T, typename T2>
    static inline bool isSameT() {
        return typeid(T) == typeid(T2);
    }
    /** @brief reinterpret_pointer_cast to boost const shared_ptr */
    template <typename T>
    static inline boost::shared_ptr<T const> reinterpretToTPtr(
        boost::shared_ptr<void const> const& value) {
        return boost::reinterpret_pointer_cast<T const, void const>(value);
    }
private:
    mutable boost::shared_mutex rwlock;
    /*boost::optional<boost::any> data;*/
    AnyDataConstType data;///< data
    boost::typeindex::stl_type_index typeinfo;///< type info
    unsigned long const addressValue;///< for compare
};
}//namespace log
}//namespace cti
