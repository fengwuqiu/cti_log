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
#include "ctilog/log/any.hpp"
namespace cti {
namespace log
{
Any::Any():
    data(nullptr),
    typeinfo(),
    addressValue(reinterpret_cast<unsigned long>(this)) {}
/** @brief copy ctor */
Any::Any(Any const& other):
    data(other.data),
    typeinfo(other.typeinfo),
    addressValue(reinterpret_cast<unsigned long>(this)) {}
Any::~Any() {}
Any& Any::operator=(Any const& other)
{
    if ((*this) == other) {
        return *this;
    }
    {
        boost::shared_lock<boost::shared_mutex> rLock(other.rwlock);
        {
            boost::upgrade_lock<boost::shared_mutex> uplock(this->rwlock);
            boost::upgrade_to_unique_lock<boost::shared_mutex> wLock(uplock);
            this->data = other.data;
            this->typeinfo = other.typeinfo;
            return *this;
        }
    }
}
bool Any::operator==(Any const& other) const
{
    return this->addressValue == other.addressValue;
}
/** @return true when is binary else return false when text */
bool Any::isBinary() const
{
    return (GetTypeInfo<std::string>() != this->typeinfo);
}
/**
 * @return {ret,ptr,size} / {ret,ptr,size,isBinary}
 * @note for string size is length()!!
 */
std::tuple<int, void const*, size_t>
Any::getDataBundle(bool* const isBinary) const
{
    if (!this->data) {
        if (isBinary) {
            *isBinary = true;
        }
        return std::tuple<int, void const*, size_t>(
            -GetErrno(Errno::NotSet),
            nullptr,
            0);
    }
    size_t size;
    void const* addr;
    if (this->isT<short>()) {
        addr = this->data.get();
        size = sizeof(int);
        if (isBinary) {
            *isBinary = true;
        }
    } else if (this->isT<int>()) {
        addr = this->data.get();
        size = sizeof(int);
        if (isBinary) {
            *isBinary = true;
        }
    } else if (this->isT<long>()) {
        addr = this->data.get();
        size = sizeof(long);
        if (isBinary) {
            *isBinary = true;
        }
    } else if (this->isT<long long>()) {
        addr = this->data.get();
        size = sizeof(long long);
        if (isBinary) {
            *isBinary = true;
        }
    } else if (this->isT<std::string>()) {
        boost::shared_ptr<std::string const> casted = Any::reinterpretToTPtr<
            std::string>(this->data);
        addr = casted->c_str();
        size = casted->length();
        if (isBinary) {
            *isBinary = false;
        }
    } else if (this->isT<AnyVector>()) {
        boost::shared_ptr<std::vector<uint8_t> const> casted =
            Any::reinterpretToTPtr<std::vector<uint8_t> >(this->data);
        addr = casted->data();
        size = casted->size();
        if (isBinary) {
            *isBinary = true;
        }
    } else if (this->isT<DataAndSize>()) {
        DataAndSizeConstPtr const casted = Any::reinterpretToTPtr<DataAndSize>(
            this->data);
        addr = casted->data;
        size = casted->size;
        if (isBinary) {
            *isBinary = true;
        }
    } else if (this->isT<Anyable>()) {
        AnyableConstPtr const casted = Any::reinterpretToTPtr<Anyable>(
            this->data);
        if (isBinary) {
            std::tie(addr, size, *isBinary) = casted->getDataBundleb();
        } else {
            std::tie(addr, size) = casted->getDataBundle();
        }
    } else {
        if (isBinary) {
            *isBinary = true;
        }
        return std::tuple<int, void const*, size_t>(
            -GetErrno(Errno::InvalidType),
            nullptr,
            0);
    }
    return std::tuple<int, void const*, size_t>(
        GetErrno(Errno::Ok),
        addr,
        size);
}
std::tuple<int, void const*, size_t, bool> Any::getDataBundleb() const
{
    int ret;
    void const* ptr;
    size_t size;
    bool isBinary;
    std::tie(ret, ptr, size) = this->getDataBundle(&isBinary);
    return std::tuple<int, void const*, size_t, bool>(
        ret,
        ptr,
        size,
        isBinary);
}
void Any::reset()
{
    boost::upgrade_lock<boost::shared_mutex> uplock(this->rwlock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> wLock(uplock);
    this->data = nullptr;
    this->typeinfo = typeid(void);
}
Any Any::create(
    void const* const data,
    size_t const size,
    boost::typeindex::stl_type_index const& typeinfo)
{
    AnyDataType value;
    if (Any::isSameT<long long>(typeinfo)) {
        value = AnyDataType(new long long(::atoll(
            reinterpret_cast<char const*>(data))));
        return Any().init<long long>(value, true);
    } else if (Any::isSameT<std::string>(typeinfo)) {
        value = AnyDataType(new std::string(reinterpret_cast<char const *>(
            data)));
        return Any().init<std::string>(value, true);
    } else if (Any::isSameT<AnyVector>(typeinfo)) {
        boost::shared_ptr<std::vector<uint8_t> > const v(
            new std::vector<uint8_t>(size));
        ::memcpy(v->data(), data, size);
        value = v;
        return Any().init<AnyVector>(value, true);
    } else {
        return Any();
    }
}
}//namespace log
}//namespace cti
