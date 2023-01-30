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
#include "ctilog/log/rwlock.hpp"
namespace cti {
namespace log
{
Rwlock::Rwlock()
{
#if defined(_WIN32) || defined(_MSC_VER)
    /* InitializeCriticalSection(&(this->cs)); */
    /* InitializeSRWLock(&(this->rwlock)); */
    ::InitializeCriticalSection(&(this->rwlock));
    this->isWr = false;
    this->reader = 0;
#else
    ::pthread_rwlock_init(&(this->rwlock), 0);
#endif
}
Rwlock::~Rwlock()
{
#if defined(_WIN32) || defined(_MSC_VER)
    /* DeleteCriticalSection(&(this->cs)); */
    ::DeleteCriticalSection(&(this->rwlock));
#else
    ::pthread_rwlock_destroy(&(this->rwlock));
#endif
}
void Rwlock::rdlock()
{
#if defined(_WIN32) || defined(_MSC_VER)
    /* AcquireSRWLockShared(&(this->rwlock)); */
    if (this->reader > 0) {

        /* someone already locked and reading
         * XXX-note: chk first !! ..
         */
        ++this->reader;
    } else {
        ::EnterCriticalSection(&(this->rwlock));
        ++this->reader;
    }
    #if defined(_WIN32) || defined(_MSC_VER)
    assert(this->reader > 0);
    #endif
#else
    ::pthread_rwlock_rdlock(&(this->rwlock));
#endif
}
void Rwlock::wrlock()
{
#if defined(_WIN32) || defined(_MSC_VER)
    /* AcquireSRWLockExclusive(&(this->rwlock)); */
    ::EnterCriticalSection(&(this->rwlock));
    this->isWr = true;// FIXME
    #if defined(_WIN32) || defined(_MSC_VER)
    assert(this->reader <= 0);
    #endif
#else
    ::pthread_rwlock_wrlock(&(this->rwlock));
#endif
}
bool Rwlock::tryRdlock()
{
#if defined(_WIN32) || defined(_MSC_VER)
    if (!this->isWr) {
        this->rdlock();
        return true;
    } else {
        return false;
    }
#else
    return ::pthread_rwlock_tryrdlock(&(this->rwlock));
#endif
}
bool Rwlock::tryWrlock()
{
#if defined(_WIN32) || defined(_MSC_VER)
    if ((!this->isWr) && (this->reader <= 0)) {
        this->wrlock();
        return true;
    } else {
        return false;
    }
#else
    return ::pthread_rwlock_trywrlock(&(this->rwlock));
#endif
}
void Rwlock::unlock()
{
#if !defined(_WIN32) && !defined(_MSC_VER)
    ::pthread_rwlock_unlock(&(this->rwlock));
#else
    if (this->isWr) {
        /* is w. itself */
        this->isWr = false;
        LeaveCriticalSection(&(this->rwlock));
    }
    else {
        /* is r. */
        if (1 == this->reader) {
            /*
             * last one to unlock
             * XXX-note. chk first !! then both next is "--"
             */
            --this->reader;
            LeaveCriticalSection(&(this->rwlock));
        } else {
            /* not last one => "--" only */
            --this->reader;
        }
        #if defined(_WIN32) || defined(_MSC_VER)
        assert(this->reader >= 0);
        #endif
    }
#endif
}
}//namespace log
}//namespace cti
