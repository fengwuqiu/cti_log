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
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <mutex>
#include <map>
#include <time.h>
#include <set>
#include <sstream>
#include "ctilog/loglevel.hpp"
#include "ctilog/log/flags.hpp"
#include "ctilog/log/scopedrwlock.hpp"

#if defined __arm__ || defined __aarch64__
#include <linux/limits.h>
#endif

namespace cti {
namespace log
{
constexpr char const* kPrimaryDefaultLogFile = "logger.log";
constexpr uint32_t kMinLogSize = 8192;
/// 256 MB / 128 MB
constexpr uint32_t kDefaultLogSize = sizeof(long) * 32 * 1024 * 1024;
struct Logger {
    /// Output type
    enum Output: uint32_t {
        CoutOrCerr = 0x1,
        File       = 0x2,
        Both       = 0x3,
    };
    /// If 0 => when get logger not change current or default
    using Outputs = Flags<Output>;
    /// Callback when set when append
    using AppendCallback = std::function<void(std::string const& name,LogLevel const& logLevel,std::string const& msg)>;
    /// Dtor to auto finish logger
    virtual ~Logger() noexcept;
    // Global configs
    // Set default logger, set only when path not empty
    static inline void setDefaultLogger(std::string const& path) noexcept;
    static inline std::string getDefaultLogger() noexcept;
    /**
     * Get a Logger instance
     * @param spinOnceLogLevel If not Unchange, will be use once
     * @param path Logger filename, if empty then will get a default Logger
     * @param outputs output config, used when create logger, if 0 then use
     * default or current exists
    */
    static Logger& getLogger(
        LogLevel const& spinOnceLogLevel = LogLevel::Unchange,
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getFatal(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getError(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getWarning(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getNote(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getInfo(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getTrace(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getDebug(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    static inline Logger& getDetail(
        std::string const& path = "",
        Outputs const& outputs = Outputs{}) noexcept;

    // Release a logger
    static void releaseLogger(std::string const& file) noexcept;
    // Instance config
    void setLogLevel(LogLevel const& logLevel) noexcept;
    // Toggle log level
    LogLevel toggleLogLevel() noexcept;
    /**
     * Set max log size
     * - If < 0 keep current max
     * - If < kMinLogSize use kMinLogSize
     */
    void setMaxSize(int32_t const maxSize) noexcept;
    inline void setOutputs(Outputs const& o) noexcept;
    inline void setAppendCallback(AppendCallback const& ac) noexcept;
    inline void enableIdx(bool const enable) noexcept;
    inline void enableTid(bool const enable) noexcept;
    /// @note copy
    std::set<std::string> getAcNameFilters() const noexcept;
    bool hasAcNameFilter(std::string const& acNameFilter) const noexcept;
    bool addAcNameFilter(std::string const& acNameFilter) noexcept;
    bool removeAcNameFilter(std::string const& acNameFilter) noexcept;
    void clearAcNameFilters() noexcept;
    /// Limit log size
    void shrinkToFit() noexcept;
    /// Check if log instance valid
    inline operator bool() const noexcept;
    /// @note Only check log level
    inline bool isLogable(LogLevel const& ll) const noexcept;
    /**
     * Rereset logger file
     * @note
     * - When param valid => will always open or reopen
     * - When param invalid => will use default or not-reopen
     */
    int64_t reset(bool const trunc = false) noexcept;
    /// Append name + file + line + msg
    int append(
        char const* const name,
        char const* const file,
        int const line,
        std::string const& msg,
        LogLevel const& logLevel) noexcept;
    /// Append a string msg
    int append(std::string const& msg, LogLevel const& logLevel) noexcept;
    // Logging methods
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& f(T const& msg)
        noexcept;
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& f(
        std::string const& name, T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& f(T const& msg)
        noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& f(
        std::string const& name, T const& msg) noexcept;
    template<uint32_t sz> Logger& f(const char(&msg)[sz]) noexcept;
    template<uint32_t sz> Logger& f(
        std::string const& name, const char(&msg)[sz]) noexcept;
    // e
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& e(T const& msg)
        noexcept;
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& e(
        std::string const& name, T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& e(T const& msg)
        noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& e(
        std::string const& name, T const& msg) noexcept;
    template<uint32_t sz> Logger& e(char const(&msg)[sz]) noexcept;
    template<uint32_t sz> Logger& e(
        std::string const& name, char const(&msg)[sz]) noexcept;
    // w
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& w(T const& msg)
        noexcept;
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& w(
        std::string const& name, T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& w(T const& msg)
        noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& w(
        std::string const& name, T const& msg) noexcept;
    template<uint32_t sz> Logger& w(char const(&msg)[sz]) noexcept;
    template<uint32_t sz> Logger& w(
        std::string const& name, char const(&msg)[sz]) noexcept;
    // n
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& n(T const& msg)
        noexcept;
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& n(
        std::string const& name, T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& n(T const& msg)
        noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& n(
        std::string const& name, T const& msg) noexcept;
    template<uint32_t sz> Logger& n(char const(&msg)[sz]) noexcept;
    template<uint32_t sz> Logger& n(
        std::string const& name, char const(&msg)[sz]) noexcept;
    // i
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& i(T const& msg)
        noexcept;
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& i(
        std::string const& name, T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& i(T const& msg)
        noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& i(
        std::string const& name, T const& msg) noexcept;
    template<uint32_t sz> Logger& i(char const(&msg)[sz]) noexcept;
    template<uint32_t sz> Logger& i(
        std::string const& name, char const(&msg)[sz]) noexcept;
    // d
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& d(T const& msg)
        noexcept;
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& d(
        std::string const& name, T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& d(T const& msg)
        noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& d(
        std::string const& name, T const& msg) noexcept;
    template<uint32_t sz> Logger& d(char const(&msg)[sz]) noexcept;
    template<uint32_t sz> Logger& d(
        std::string const& name, char const(&msg)[sz]) noexcept;
    // <<
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& operator<<(
        T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& operator<<(
        T const& msg) noexcept;
    template<uint32_t sz>
    Logger& operator<<(char const(&msg)[sz]) noexcept;
    // ,
    template<typename T = std::string>
    typename std::enable_if<std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& operator,(
        T const& msg) noexcept;
    template<typename T>
    typename std::enable_if<!std::is_same<std::string,
        typename std::decay<T>::type>::value, Logger>::type& operator,(
        T const& msg) noexcept;
    template<uint32_t sz>
    Logger& operator,(char const(&msg)[sz]) noexcept;
    /// Finish log
    void finish() noexcept;
protected:
    static std::string defaultLogFile;// Some global options
    static Logger& hasLogger(std::string const& file) noexcept;
    /**
     * Create a logger
     * @param path if path empty only for internal emptyLogger
     * @param outputs if 0 then use default
     * @param maxSize if < 0 use kDefaultLogSize, else min kMinLogSize
     * @param trunc true to trunc file, used only when path not nil and
     * Output::File set
     */
    Logger(std::string const& path,Outputs const& outputs = Outputs{},int32_t const maxSize = -1,bool const trunc = false) noexcept;
    Logger(Logger const&) = delete;
    Logger& operator=(Logger const&) = delete;
    void tryDoAcCb(std::string const& name, LogLevel const& logLevel, std::string const &msg) const noexcept;
    //Logger instances and related
    static Logger emptyLogger;
    static boost::shared_mutex instancesRwlock;
    static std::map<std::string, boost::shared_ptr<Logger>> instances;
    //Instance properties
    mutable std::mutex writemutex;
    LogLevel logLevel { LogLevel::Note };
    LogLevel spinOnceLogLevel{ LogLevel::Unchange };
    FILE* log{ nullptr };
    std::string const path;
    Outputs outputs{ Output::CoutOrCerr };
    uint32_t maxSize{ kDefaultLogSize };
    bool hasIdx{ true }; //序列号
    bool hasTid{ false };//线程id
    AppendCallback appendCallback{ nullptr };
    mutable boost::shared_mutex acNameFiltersRwlock;
    /// @note empty name to filter nil and empty
    std::set<std::string> acNameFilters;
};
//--
extern std::string LogRealTime() noexcept;
inline void Logger::setDefaultLogger(std::string const& path) noexcept
{
    if (!path.empty()) {
        Logger::defaultLogFile = path;
    }
}
inline std::string Logger::getDefaultLogger() noexcept
{
    return Logger::defaultLogFile;
}

inline Logger::operator bool() const noexcept
{
    return !this->path.empty();
}
inline bool Logger::isLogable(LogLevel const& ll) const noexcept
{
    return this->logLevel >= ll;
}
inline void Logger::setOutputs(Outputs const& o) noexcept
{
    this->outputs = o;
}
inline void Logger::setAppendCallback(AppendCallback const& ac) noexcept
{
    this->appendCallback = ac;
}
inline void Logger::enableIdx(bool const enable) noexcept
{
    this->hasIdx = enable;
}
inline void Logger::enableTid(bool const enable) noexcept
{
    this->hasTid = enable;
}
constexpr inline LogLevel GetNextLogLevel(LogLevel const& logLevel) noexcept
{
    return static_cast<LogLevel>(static_cast<uint32_t>(logLevel) + 1);
}
static inline LogLevel& ToNextLogLevel(LogLevel& logLevel) noexcept
{
    return logLevel = GetNextLogLevel(logLevel);
}
//--
inline Logger& Logger::getFatal(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Fata, path, outputs);
}
inline Logger& Logger::getError(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Erro, path, outputs);
}
inline Logger& Logger::getWarning(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Warn, path, outputs);
}
inline Logger& Logger::getNote(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Note, path, outputs);
}
inline Logger& Logger::getInfo(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Info, path, outputs);
}
inline Logger& Logger::getTrace(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Trac, path, outputs);
}
inline Logger& Logger::getDebug(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Debu, path, outputs);
}
inline Logger& Logger::getDetail(std::string const& path, Outputs const& outputs) noexcept
{
    return Logger::getLogger(LogLevel::Deta, path, outputs);
}

/**********/
/*std::is_same 判断类型是否一致  std::decay 退化类型的修饰*/
/*std::enable_if判断退化后的模板参数T与std::string类型一致,则使用Logger作为返回参数*/
// f
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::f(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Fata);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::f(std::string const& name, T const& msg) noexcept
{
    this->append(name.c_str(), nullptr, -1, msg, LogLevel::Fata);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::f(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), LogLevel::Fata);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string, typename std::decay<T>::type>::value, Logger>::type&
Logger::f(std::string const& name, T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(name.c_str(), nullptr, -1, ss.str(), LogLevel::Fata);
    return *this;
}
template<uint32_t sz> Logger&
Logger::f(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Fata);
    return *this;
}
template<uint32_t sz> Logger&
Logger::f(std::string const& name, char const(&msg)[sz]) noexcept
{
    this->append(name, nullptr, -1, msg, LogLevel::Fata);
    return *this;
}
// e
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::e(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Erro);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::e(std::string const& name, T const& msg) noexcept
{
    this->append(name.c_str(), nullptr, -1, msg, LogLevel::Erro);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::e(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), LogLevel::Erro);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::e(std::string const& name, T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(name.c_str(), nullptr, -1, ss.str(), LogLevel::Erro);
    return *this;
}
template<uint32_t sz> Logger&
Logger::e(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Erro);
    return *this;
}
template<uint32_t sz> Logger&
Logger::e(std::string const& name, char const(&msg)[sz]) noexcept
{
    this->append(name, nullptr, -1, msg, LogLevel::Erro);
    return *this;
}
// w
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::w(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Warn);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::w(std::string const& name, T const& msg) noexcept
{
    this->append(name.c_str(), nullptr, -1, msg, LogLevel::Warn);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::w(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), LogLevel::Warn);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::w(std::string const& name, T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(name.c_str(), nullptr, -1, ss.str(), LogLevel::Warn);
    return *this;
}
template<uint32_t sz> Logger&
Logger::w(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Warn);
    return *this;
}
template<uint32_t sz> Logger&
Logger::w(std::string const& name, char const(&msg)[sz]) noexcept
{
    this->append(name, nullptr, -1, msg, LogLevel::Warn);
    return *this;
}
// n
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::n(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Note);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::n(std::string const& name, T const& msg) noexcept
{
    this->append(name.c_str(), nullptr, -1, msg, LogLevel::Note);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::n(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), LogLevel::Note);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::n(std::string const& name, T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(name.c_str(), nullptr, -1, ss.str(), LogLevel::Note);
    return *this;
}
template<uint32_t sz> Logger&
Logger::n(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Note);
    return *this;
}
template<uint32_t sz> Logger&
Logger::n(std::string const& name, char const(&msg)[sz]) noexcept
{
    this->append(name, nullptr, -1, msg, LogLevel::Note);
    return *this;
}
// i
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::i(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Info);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::i(std::string const& name, T const& msg) noexcept
{
    this->append(name.c_str(), nullptr, -1, msg, LogLevel::Info);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::i(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), LogLevel::Info);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::i(std::string const& name, T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(name.c_str(), nullptr, -1, ss.str(), LogLevel::Info);
    return *this;
}
template<uint32_t sz> Logger&
Logger::i(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Info);
    return *this;
}
template<uint32_t sz> Logger&
Logger::i(std::string const& name, char const(&msg)[sz]) noexcept
{
    this->append(name, nullptr, -1, msg, LogLevel::Info);
    return *this;
}
// d
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::d(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Debu);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::d(std::string const& name, T const& msg) noexcept
{
    this->append(name.c_str(), nullptr, -1, msg, LogLevel::Debu);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::d(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), LogLevel::Debu);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::d(std::string const& name, T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(name.c_str(), nullptr, -1, ss.str(), LogLevel::Debu);
    return *this;
}
template<uint32_t sz> Logger&
Logger::d(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, LogLevel::Debu);
    return *this;
}
template<uint32_t sz> Logger&
Logger::d(std::string const& name, char const(&msg)[sz]) noexcept
{
    this->append(name, nullptr, -1, msg, LogLevel::Debu);
    return *this;
}
// <<
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::operator<<(T const& msg) noexcept
{
    this->append(nullptr, nullptr, -1, msg, this->logLevel);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::operator<<(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(nullptr, nullptr, -1, ss.str(), this->logLevel);
    return *this;
}
template<uint32_t sz> Logger&
Logger::operator<<(char const(&msg)[sz]) noexcept
{
    this->append(nullptr, nullptr, -1, msg, this->logLevel);
    return *this;
}
template<typename T>
typename std::enable_if<std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::operator,(T const& msg) noexcept
{
    this->append(msg, this->logLevel);
    return *this;
}
template<typename T>
typename std::enable_if<!std::is_same<std::string,typename std::decay<T>::type>::value, Logger>::type&
Logger::operator,(T const& msg) noexcept
{
    std::stringstream ss;
    ss << msg;
    this->append(ss.str(), this->logLevel);
    return *this;
}
template<uint32_t sz> Logger&
Logger::operator,(char const(&msg)[sz]) noexcept
{
    this->append(msg, this->logLevel);
    return *this;
}
//---------------
}//namespace log
}//namespace cti
#ifndef CTI_BASE_LOG_HPP
#   define CTI_BASE_LOG_HPP 1
#endif


