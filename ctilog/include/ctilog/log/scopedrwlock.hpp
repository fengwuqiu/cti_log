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
#include "boost/thread/shared_mutex.hpp"
#include "rwlock.hpp"
namespace cti {
namespace log
{
/// Auto unlock guard for read lock
struct ScopedReadLock {
    ScopedReadLock(Rwlock& lock): lock(lock) {
        this->lock.rdlock();
    }
    virtual ~ScopedReadLock() {
        this->lock.unlock();
    }
    // do not try copy
    ScopedReadLock(ScopedReadLock const&) = delete;
    ScopedReadLock& operator=(ScopedReadLock const&) = delete;
private:
    Rwlock& lock;
};
/// Auto unlock guard for write lock
struct ScopedWriteLock {
    ScopedWriteLock(Rwlock& lock): lock(lock) {
        this->lock.wrlock();
    }
    virtual ~ScopedWriteLock() {
        this->lock.unlock();
    }
    // do not try copy
    ScopedWriteLock(ScopedWriteLock const&) = delete;
    ScopedWriteLock& operator=(ScopedWriteLock const&);
private:
    Rwlock& lock;
};
/**
 * @note better:
boost::shared_lock<boost::shared_mutex> readLock(this->xyzRwlock);
 */
struct BoostScopedReadLock {
    BoostScopedReadLock(boost::shared_mutex& rwlock): readLock(rwlock) {}
    ~BoostScopedReadLock() {}
    BoostScopedReadLock(BoostScopedReadLock const& other) = delete;
    BoostScopedReadLock& operator=(BoostScopedReadLock const& other) = delete;
private:
    boost::shared_lock<boost::shared_mutex> readLock;
};
/**
 * @note better:
boost::upgrade_lock<boost::shared_mutex> uplock(this->xyzRwlock);
boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(uplock);
 */
struct BoostScopedWriteLock {
    BoostScopedWriteLock(boost::shared_mutex& rwlock):uplock(rwlock), writeLock(uplock) {}
    ~BoostScopedWriteLock() {}
    BoostScopedWriteLock(BoostScopedWriteLock const& other) = delete;
    BoostScopedWriteLock& operator=(BoostScopedWriteLock const& other) = delete;
private:
    boost::upgrade_lock<boost::shared_mutex> uplock;
    boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock;
};
}//namespace log
}//namespace cti
