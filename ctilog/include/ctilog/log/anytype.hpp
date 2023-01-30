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
#include <vector>
#include "boost/type_index/stl_type_index.hpp"
#include "boost/shared_ptr.hpp"
namespace cti {
namespace log
{
template <typename T>
static inline boost::typeindex::stl_type_index GetTypeInfo()
{
    return typeid(T);
}
struct Anyable;
typedef boost::shared_ptr<Anyable> AnyablePtr;
typedef boost::shared_ptr<Anyable const> AnyableConstPtr;
struct Anyable {
    Anyable() {}
    virtual ~Anyable() {}
    virtual AnyablePtr clone() const = 0;
    /**
     * @returns {ptr, size} / {ptr, size, isBinary}
     * @note all in binary is ok
     */
    virtual std::tuple<void const*, size_t> getDataBundle() const = 0;
    virtual std::tuple<void const*, size_t, bool> getDataBundleb() const = 0;
    virtual boost::typeindex::stl_type_index getTypeInfo() const = 0;
};
struct DataAndSize {
    void const* data;
    size_t size;
    DataAndSize(): data(nullptr), size(0) {}
    DataAndSize(void const* const mem, size_t const size):
        data(mem), size(size) {}
};
struct AnyableDataAndSize: public DataAndSize, Anyable {
    AnyableDataAndSize(): DataAndSize() {}
    AnyableDataAndSize(void const* const mem, size_t const size):
        DataAndSize(mem, size) {}
    virtual AnyablePtr clone() const;
    /**
     * @returns {ptr, size} / {ptr, size, isBinary}
     * @note all in binary is ok
     */
    virtual std::tuple<void const*, size_t> getDataBundle() const;
    virtual std::tuple<void const*, size_t, bool> getDataBundleb() const;
    virtual boost::typeindex::stl_type_index getTypeInfo() const;
};
typedef boost::shared_ptr<DataAndSize> DataAndSizePtr;
typedef boost::shared_ptr<DataAndSize const> DataAndSizeConstPtr;

template<typename T>
struct AnyableVector: public std::vector<T>, Anyable {
    AnyableVector(std::initializer_list<T> init): std::vector<T>(init) {}
    virtual AnyablePtr clone() const {
        AnyablePtr const ret(new AnyableVector(*this));
        return ret;
    }
    /**
     * @returns {ptr, size} / {ptr, size, isBinary}
     * @note all in binary is ok
     */
    virtual std::tuple<void const*, size_t> getDataBundle() const {
        return std::tuple<void const*, size_t>(
            this->data(),
            this->size());
    }
    virtual std::tuple<void const*, size_t, bool> getDataBundleb() const {
        return std::tuple<void const*, size_t, bool>(
            this->data(),
            this->size(),
            true);
    }
    virtual boost::typeindex::stl_type_index getTypeInfo() const {
        return typeid(AnyableVector<T>);
    }
};
}//namespace log
}//namespace cti
