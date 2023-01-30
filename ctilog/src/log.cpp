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
#include "ctilog/log.hpp"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sstream>
#include <atomic>
#include "ctilog/log/file.hpp"
namespace cti {
namespace log
{
/// For debug
LogLevel kLogLevel = LogLevel::Note;
std::string Logger::defaultLogFile = kPrimaryDefaultLogFile;
boost::shared_mutex Logger::instancesRwlock;
std::map<std::string, boost::shared_ptr<Logger>> Logger::instances;
Logger Logger::emptyLogger("");

Logger& Logger::hasLogger(std::string const& file) noexcept
{
    //lock
    BoostScopedReadLock readLock(Logger::instancesRwlock);
    //find the logger
    auto it = Logger::instances.find(file);
    if (it == Logger::instances.end()) {
        return emptyLogger;
    }
    return *(it->second);
}

Logger& Logger::getLogger(LogLevel const& spinOnceLogLevel, std::string const& path, Outputs const& outputs) noexcept
{
    std::string file;
    if (path.empty()) {
        file = Logger::defaultLogFile;
    } else {
        file = path;
    }

    auto& l = Logger::hasLogger(file);
    if (&Logger::emptyLogger != &l) {
        if (LogLevel::Unchange != spinOnceLogLevel) {
            l.spinOnceLogLevel = spinOnceLogLevel;
        }
        if (outputs.testFlag(Output::CoutOrCerr) || outputs.testFlag(Output::File)) {
            l.outputs = outputs;
        }
        return l;
    }
    BoostScopedWriteLock writeLock(Logger::instancesRwlock);
    auto const ret = boost::shared_ptr<Logger>(new Logger(file, outputs));
    Logger::instances[file] = ret;
    if (LogLevel::Unchange != spinOnceLogLevel) {
        ret->spinOnceLogLevel = spinOnceLogLevel;
    }
    if (outputs.testFlag(Output::CoutOrCerr) || outputs.testFlag(Output::File)) {
        ret->outputs = outputs;
    }
    return *(ret);
}

void Logger::releaseLogger(std::string const& file) noexcept
{
    BoostScopedWriteLock writeLock(Logger::instancesRwlock);
    instances.erase(file);
}

Logger::Logger(std::string const& path,Outputs const& outputs,int32_t const maxSize,bool const trunc) noexcept: path(path)
{
    this->setMaxSize(maxSize);
    if (!path.empty() && outputs.testFlag(Output::File)) {
        if (this->reset(trunc) >= 0) {
            this->shrinkToFit();
        }
    }
    if (!outputs) {
        return;
    }
    this->outputs = outputs;
}
Logger::~Logger() noexcept
{
    this->finish();
}
void Logger::finish() noexcept
{
    if (this->path.empty()) {
        return;
    }
    std::unique_lock<std::mutex> lock(Logger::writemutex);
    if (!this->log) {
        return;
    }
    ::fflush(this->log);
    ::fclose(this->log);
    this->log = nullptr;
}
void Logger::setLogLevel(LogLevel const& logLevel) noexcept
{
    if (logLevel >= LogLevel::Min && logLevel <= LogLevel::Max) {
        this->logLevel = logLevel;
    }
}
LogLevel Logger::toggleLogLevel() noexcept
{
    uint32_t lv = uint32_t(this->logLevel);
    lv += 1;
    if (lv > uint32_t(LogLevel::Max)) {
        lv = uint32_t(LogLevel::Min);
    }
    return this->logLevel = static_cast<LogLevel>(lv);
}
//set file max size
void Logger::setMaxSize(int32_t const maxSize) noexcept
{
   if (maxSize >= 0) {
       if (uint32_t(maxSize) < kMinLogSize) {
           this->maxSize = kMinLogSize;
       } else {
           this->maxSize = maxSize;
       }
   }
}
int64_t Logger::reset(bool const trunc) noexcept
{
    // Skip empty logger
    if (this->path.empty()) {
        return -EPERM;
    }
    {
        // Finish when not need Output::File
        auto const o = this->outputs;
        if (!o.testFlag(Output::File)) {
            this->finish();
            return 0;
        }
    }
    {
        std::unique_lock<std::mutex> lock(this->writemutex);
        if (this->log) {
            // Opened but means to open new => close old => always
            ::fflush(this->log);
            ::fclose(this->log);
            this->log = nullptr;
        }
        // Mkdir
        {
            std::vector<char> openf2(this->path.length() + 1);
            ::memcpy(openf2.data(), this->path.data(), openf2.size());
            int const ret = MkDirs(::dirname(openf2.data()));
            if (ret < 0) {
                // Ignore error
                std::cerr << "Logger::reset: MkDirs fail: " << strerror(-ret)
                    << "\n";
            }
        }
        // open
        if (trunc) {
            this->log = ::fopen(this->path.c_str(), "wb");
        } else {
            this->log = ::fopen(this->path.c_str(), "ab");
        }
        if (!this->log) {
            // Open fail
            int ret = errno;
            if (!ret) {
                // Access fail
                ret = EACCES;
            }
            return -ret;
        }
    } /* ScopedLock */
    // Final try limit log size ? X
    return 0;// OK
}
constexpr uint32_t kShrinkToFitInteval = 5;
constexpr uint32_t kFlushInteval = 3;
static std::atomic<uint64_t> kLogIdx(0);
int Logger::append(char const* const name,char const* const file,int const line,std::string const& msg,LogLevel const& logLevel) noexcept
{
    if (this->path.empty()) {
        return -EPERM;
    }
    LogLevel lvl = this->spinOnceLogLevel;
    if (LogLevel::Unchange != lvl) {
        this->spinOnceLogLevel = LogLevel::Unchange;
        if (this->logLevel < lvl) {
            return 0;
        }
    }
    else {
        if (this->logLevel < logLevel) {
            return 0;
        }
        lvl = logLevel;
    }
    // If not need
    auto const o = this->outputs;
    if (!o) {
        return ENODEV;
    }
    // Reset log file when has file output and no log file
    if (o.testFlag(Output::File)) {
        if (!this->log) {
            if (this->reset(false) < 0) {
                std::cerr << "Logger::append: Output has file but cannot get "
                    "log\n";
                if (!o.testFlag(Output::CoutOrCerr)) {
                    // Done when file fail and no CoutOrCerr output
                    return -EFAULT;
                }
            }
            else {
                // Fit when create
                this->shrinkToFit();
            }
        }
    }
    // Append
    ++kLogIdx;
    int64_t ret;
    // Msg to write(append)
    std::string w;
    {
        std::unique_lock<std::mutex> lock(this->writemutex);
        if (this->hasIdx) {
            w += std::to_string(kLogIdx);
        }
        w += "[" + LogRealTime() + " ";
        if (this->hasTid) {
            w += std::to_string(uint64_t(::pthread_self())) + " ";
        }
        w += logLevelToString(lvl) + "]";
        if (name) {
            w += "[";
            w += name;
            w += "]";
        }
        w += " " + msg;
        if (file) {
            w += " (";
            w += file;
            if (line >= 0) {
                w += "+";
                w += std::to_string(line);
            }
            w += ")";
        }
        if (o.testFlag(Output::CoutOrCerr)) {
            switch (lvl) {
            // Just color
#           define LOGFALL(__fmt, __args...) \
                fprintf(stdout, "\033[30;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFFATA(__fmt, __args...) \
                fprintf(stderr, "\033[1;31;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFERRO(__fmt, __args...) \
                fprintf(stderr, "\033[31;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFWARN(__fmt, __args...) \
                fprintf(stderr, "\033[33;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFNOTE(__fmt, __args...) \
                fprintf(stdout, "\033[1;30;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFINFO(__fmt, __args...) \
                fprintf(stdout, __fmt "\n", ##__args)
#           define LOGFTRAC(__fmt, __args...) \
                fprintf(stdout, "\033[34;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFDEBU(__fmt, __args...) \
                fprintf(stdout, "\033[36;49m" __fmt "\033[0m\n", ##__args)
#           define LOGFDETA(__fmt, __args...) \
                fprintf(stdout, __fmt "\n", ##__args)
            case LogLevel::Fata: LOGFFATA("%s", w.c_str()); break;
            case LogLevel::Erro: LOGFERRO("%s", w.c_str()); break;
            case LogLevel::Warn: LOGFWARN("%s", w.c_str()); break;
            case LogLevel::Note: LOGFNOTE("%s", w.c_str()); break;
            case LogLevel::Info: LOGFINFO("%s", w.c_str()); break;
            case LogLevel::Trac: LOGFTRAC("%s", w.c_str()); break;
            case LogLevel::Debu: LOGFDEBU("%s", w.c_str()); break;
            case LogLevel::Deta: LOGFDETA("%s", w.c_str()); break;
            default:             LOGFALL("%s", w.c_str()); break;
#           undef LOGFALL
#           undef LOGFFATA
#           undef LOGFERRO
#           undef LOGFWARN
#           undef LOGFNOTE
#           undef LOGFINFO
#           undef LOGFTRAC
#           undef LOGFDEBU
#           undef LOGFDETA
            }
            ret = 0;
        }
        if (o.testFlag(Output::File)) {
            if (!this->log) {
                ret = -ENOENT;
                goto end;
            }
            ret = ::fwrite(w.c_str(), 1, w.length(), this->log);
            ::fwrite("\n", 1, 1, this->log);
            if (0 == (kLogIdx % kFlushInteval)) {
                ::fflush(this->log);
            }
            if (ret != int64_t(w.length())) {
                ret = -errno;
                if (!ret) {
                    ret = -EIO;
                }
                goto end;
            }
        }
    } /* ScopedLock */
end:
    if (ret < 0) {
        this->reset(false);
    } else if (0 == (kLogIdx % kShrinkToFitInteval)) {
        this->shrinkToFit();
    }
    // Callback when need
    if (name) {
        this->tryDoAcCb(name, lvl, w);
    } else {
        this->tryDoAcCb("", lvl, w);
    }
    return ret;
}
int Logger::append(std::string const& msg, LogLevel const& logLevel) noexcept
{
    if (this->path.empty()) {
        return -EPERM;
    }
    LogLevel lvl = this->spinOnceLogLevel;
    if (LogLevel::Unchange != lvl) {
        this->spinOnceLogLevel = LogLevel::Unchange;
        if (this->logLevel < lvl) {
            return 0;
        }
    }
    else {
        if (this->logLevel < logLevel) {
            return 0;
        }
        lvl = logLevel;
    }
    auto const o = this->outputs;
    if (!o) {
        return ENODEV;
    }
    {
        std::unique_lock<std::mutex> lock(this->writemutex);
        if (o.testFlag(Output::File) && !o.testFlag(Output::CoutOrCerr)
            && !this->log) {
            std::cerr << "Logger::append: only output file enabled but no "
                "log file \n";
            return -ENOENT;
        }
        if (o.testFlag(Output::CoutOrCerr)) {
            switch (lvl) {
            // Just color
#           define LOGFALL(__fmt, __args...) \
                fprintf(stdout, "\033[30;49m" __fmt "\033[0m", ##__args)
#           define LOGFFATA(__fmt, __args...) \
                fprintf(stderr, "\033[1;31;49m" __fmt "\033[0m", ##__args)
#           define LOGFERRO(__fmt, __args...) \
                fprintf(stderr, "\033[31;49m" __fmt "\033[0m", ##__args)
#           define LOGFWARN(__fmt, __args...) \
                fprintf(stderr, "\033[33;49m" __fmt "\033[0m", ##__args)
#           define LOGFNOTE(__fmt, __args...) \
                fprintf(stdout, "\033[1;30;49m" __fmt "\033[0m", ##__args)
#           define LOGFINFO(__fmt, __args...) \
                fprintf(stdout, __fmt, ##__args)
#           define LOGFTRAC(__fmt, __args...) \
                fprintf(stdout, "\033[34;49m" __fmt "\033[0m", ##__args)
#           define LOGFDEBU(__fmt, __args...) \
                fprintf(stdout, "\033[36;49m" __fmt "\033[0m", ##__args)
#           define LOGFDETA(__fmt, __args...) \
                fprintf(stdout, __fmt, ##__args)
            case LogLevel::Fata: LOGFFATA("%s", msg.c_str()); break;
            case LogLevel::Erro: LOGFERRO("%s", msg.c_str()); break;
            case LogLevel::Warn: LOGFWARN("%s", msg.c_str()); break;
            case LogLevel::Note: LOGFNOTE("%s", msg.c_str()); break;
            case LogLevel::Info: LOGFINFO("%s", msg.c_str()); break;
            case LogLevel::Trac: LOGFTRAC("%s", msg.c_str()); break;
            case LogLevel::Debu: LOGFDEBU("%s", msg.c_str()); break;
            case LogLevel::Deta: LOGFDETA("%s", msg.c_str()); break;
            default:             LOGFALL("%s", msg.c_str()); break;
#           undef LOGFALL
#           undef LOGFFATA
#           undef LOGFERRO
#           undef LOGFWARN
#           undef LOGFNOTE
#           undef LOGFINFO
#           undef LOGFTRAC
#           undef LOGFDEBU
#           undef LOGFDETA
            }
        }
        if (o.testFlag(Output::File)) {
            if (!this->log) {
                return -ENOENT;
            }
            int64_t ret = ::fwrite(msg.c_str(), 1, msg.length(), this->log);
            if (0 == (kLogIdx % kFlushInteval)) {
                ::fflush(this->log);
            }
            if (ret != int64_t(msg.length())) {
                ret = -errno;
                if (!ret) {
                    ret = -EIO;
                }
                return ret;
            }
        }
    } /* ScopedLock */
    // Callback when need
    this->tryDoAcCb("", lvl, msg);
    return 0;
}
void Logger::tryDoAcCb(std::string const& name, LogLevel const& logLevel, std::string const &msg) const noexcept
{
    if (auto const ac = this->appendCallback)
    {
        {
            BoostScopedReadLock readLock(this->acNameFiltersRwlock);
            if (this->acNameFilters.empty()) {
                return;
            }
            if (this->acNameFilters.cend() == this->acNameFilters.find(name)) {
                return;
            }
        }
        try {
            ac(name, logLevel, msg);
        } catch(...) {}
    }
}
//--AcNameFilter
std::set<std::string> Logger::getAcNameFilters() const noexcept
{
    BoostScopedReadLock readLock(this->acNameFiltersRwlock);
    return this->acNameFilters;
}
bool Logger::hasAcNameFilter(std::string const& acNameFilter) const noexcept
{
    BoostScopedReadLock readLock(this->acNameFiltersRwlock);
    return this->acNameFilters.cend() != this->acNameFilters.find(acNameFilter);
}
bool Logger::addAcNameFilter(std::string const& acNameFilter) noexcept
{
    BoostScopedWriteLock writeLock(this->acNameFiltersRwlock);
    if (this->acNameFilters.end() != this->acNameFilters.find(acNameFilter)) {
        return false;
    }
    this->acNameFilters.insert(acNameFilter);
    return true;
}
bool Logger::removeAcNameFilter(std::string const& acNameFilter) noexcept
{
    BoostScopedWriteLock writeLock(this->acNameFiltersRwlock);
    if (this->acNameFilters.end() == this->acNameFilters.find(acNameFilter)) {
        return false;
    }
    this->acNameFilters.erase(acNameFilter);
    return true;
}
void Logger::clearAcNameFilters() noexcept
{
    BoostScopedWriteLock writeLock(this->acNameFiltersRwlock);
    if (!this->acNameFilters.empty()) {
        this->acNameFilters.clear();
    }
}

//调整文件大小
/*
void Logger::shrinkToFit() noexcept
{
    if (this->path.empty()) {
        return;
    }
    std::string tmpFilename;
    {
        std::unique_lock<std::mutex> lock(this->writemutex);
        // Chk if no log
        if (!this->log) {
            return;
        }
        off_t const size = GetFileSize(this->path); //文件大小
        if (size < 0) {
            // Fail ignore
            std::cerr << "Logger::shrinkToFit:  GetFileSize fail\n";
            return;
        }
        if (static_cast<uint64_t>(size) <= this->maxSize) {
            return;
        }
        // Current size => max / 2
        std::cout << "Logger::shrinkToFit: will limitSize " << size << " to half of max " << this->maxSize << "\n";
        // Open to read last maxSize / 2 bytes
        log::File currentLog(this->path);
        int32_t code = currentLog.open(FileOpenConfig{PosixFileAccessMode::ReadOnly});
        if (code < 0) {
            std::cerr << "Logger::shrinkToFit: cannot open log file: " << code << "\n";
            return;
        }
        // Jump to last maxSize / 2
        currentLog.rjump2Offset(-int64_t(this->maxSize / 2));
        // Open a tmp file to save old file last: max / 2 bytes
        tmpFilename = this->path + ".logger.swp";
        PosixFileModes modes(0644);
        log::File tmpFile(tmpFilename);
        if (tmpFile.open(FileOpenConfig{PosixFileAccessMode::WriteOnly,SimplifiedFileOpenFlag::Create,&modes }) < 0) {
            std::cerr << "Logger::shrinkToFit: cannot open tmp file: " << tmpFilename << "\n";
            goto rmtmp;
        }
        // Copy
        auto const doWrite2Tmp = [&code, &tmpFile](uint8_t const* const data,
            uint32_t const size) -> bool {
            uint64_t w;
            std::tie(code, w) = tmpFile.write(data, size);
            if (code < 0) {
                std::cerr << "Logger::shrinkToFit: write fail!\n";
                return true;// Done
            } else {
                return false;
            }
        };
        uint64_t wroteBytes;
        std::tie(code, wroteBytes) = currentLog.traverse(doWrite2Tmp, kBigPerReadBytes, this->maxSize / 2);
        currentLog.close();
        tmpFile.close();
        if (tmpFile.open(FileOpenConfig{ PosixFileAccessMode::ReadOnly}) < 0) {
            std::cerr << "Logger::shrinkToFit: cannot open tmp file!\n";
            goto rmtmp;
        }
        if (code >= 0) {
            // Write OK then move tmp to log
            ::fclose(this->log);
            this->log = nullptr;
            // Move
            if (currentLog.open(FileOpenConfig{PosixFileAccessMode::WriteOnly,SimplifiedFileOpenFlag::Create,&modes }) < 0) {
                std::cerr << "Logger::shrinkToFit: cannot open log file\n";
                goto rmtmp;
            }
            auto const doWrite2Log = [&code, &currentLog](uint8_t const* const data, uint32_t const size) -> bool {
                uint64_t w;
                std::tie(code, w) = currentLog.write(data, size);
                if (code < 0) {
                    std::cerr << "Logger::shrinkToFit: write fail\n";
                    return true;// Done
                } else {
                    return false;
                }
            };
            std::tie(code, wroteBytes) = tmpFile.traverse(doWrite2Log);
            currentLog.close();
            // New log
            FILE* newLog = ::fopen(this->path.c_str(), "ab");
            if (!newLog) {
                std::cerr << "Logger::shrinkToFit: cannot open log\n";
                goto rmtmp;
            }
            // Done
            this->log = newLog;
        }
    } // ScopedLock
rmtmp:
    RemoveFiles(tmpFilename);
}*/
//--
void Logger::shrinkToFit() noexcept
{
    if (this->path.empty()) {
        return;
    }
    std::string tmpFilename;
    {
        std::unique_lock<std::mutex> lock(this->writemutex);
        // Chk if no log
        if (!this->log) {
            return;
        }
        off_t const size = GetFileSize(this->path); //文件大小
        if (size < 0) {
            // Fail ignore
            std::cerr << "Logger::shrinkToFit:  GetFileSize fail\n";
            return;
        }
        if (static_cast<uint64_t>(size) <= this->maxSize/2) {
            return;
        }
        std::cout << "Logger::shrinkToFit: will limitSize " << size << " to half of max " << this->maxSize << "\n";
        ::fflush(this->log);
        // Open to read last maxSize / 2 bytes
        log::File currentLog(this->path);
        int32_t code = currentLog.open(FileOpenConfig{PosixFileAccessMode::ReadOnly});
        if (code < 0) {
            std::cerr << "Logger::shrinkToFit: cannot open log file: " << code << "\n";
            return;
        }
        // Jump to last begin
        currentLog.jump2Begin();
        // Open a tmp file to save old file last: max / 2 bytes
        tmpFilename = this->path + ".1";
        PosixFileModes modes(0644);
        log::File tmpFile(tmpFilename);
        if(IsExists(tmpFilename)){
            RemoveFiles(tmpFilename);
        }
        if (tmpFile.open(FileOpenConfig{PosixFileAccessMode::ReadWrite,SimplifiedFileOpenFlag::Create,&modes}) < 0) {
            std::cerr << "Logger::shrinkToFit: cannot open tmp file: " << tmpFilename << "\n";
            return;
        }
        // Copy
        auto const doWrite2Tmp = [&code, &tmpFile](uint8_t const* const data,
            uint32_t const size) -> bool {
            uint64_t w;
            std::tie(code, w) = tmpFile.write(data, size);
            if (code < 0) {
                std::cerr << "Logger::shrinkToFit: write fail!\n";
                return true;// Done
            } else {
                return false;
            }
        };
        uint64_t wroteBytes;
        std::tie(code, wroteBytes) = currentLog.traverse(doWrite2Tmp, kBigPerReadBytes, this->maxSize);
        currentLog.close();
        tmpFile.close();
        if (code >= 0) {
            if(::fclose(this->log) < 0){
                ::fclose(this->log);
            }
            this->log = nullptr;
            // New log
            FILE* newLog = ::fopen(this->path.c_str(), "wb");
            if (!newLog) {
                std::cerr << "Logger::shrinkToFit: cannot open log\n";
                newLog = ::fopen(this->path.c_str(), "wb");
                if(!newLog){
                    return;
                }
            }
            // Done
            this->log = newLog;
        }
    } // ScopedLock
}
//转成字符
std::string logLevelToString(LogLevel const& logLevel) noexcept
{
    switch (logLevel) {
#   define __CASE(c) case LogLevel::c: return #c "(" + \
        std::to_string(uint32_t(logLevel)) + ")";
    __CASE(Fata)
    __CASE(Erro)
    __CASE(Warn)
    __CASE(Note)
    __CASE(Info)
    __CASE(Trac)
    __CASE(Debu)
    __CASE(Deta)
#   undef __CASE
    default: return "(unknown)(" + std::to_string(uint32_t(logLevel)) + ")";
    }
}
//时间
std::string LogRealTime() noexcept
{
    timespec tp;
    if (::clock_gettime(CLOCK_REALTIME_COARSE, &tp)) {
        return "0000 00-00-00 00:00:00.000000000";
    }
    struct tm localctm;
#if !defined _WIN32 || !_WIN32
    struct tm* const chk = ::localtime_r(&tp.tv_sec, &localctm);
#else
    struct tm* const chk = ::localtime(&tp.tv_sec);
    if (!chk) {
        ::memset(&localctm, chk, sizeof(struct tm));
    }
#endif
    if (!chk) {
        ::memset(&localctm, 0, sizeof(struct tm));
    }
    if (0 == localctm.tm_mday) {
        localctm.tm_mday = 1;
    }
    //--
    int const tz = int(int64_t(localctm.tm_gmtoff / 3600.0));
    char buf[40];
    ::snprintf(buf, sizeof(buf),"%02d %04d-%02d-%02d %02d:%02d:%02d.%09ld",
        tz & 0xff,
        localctm.tm_year + 1900,
        localctm.tm_mon + 1,
        localctm.tm_mday,
        localctm.tm_hour,
        localctm.tm_min,
        localctm.tm_sec,
#if !defined __APPLE__
        tp.tv_nsec
#else
        long(tp.tv_nsec)
#endif
    );
    return buf;
}

}//namespace log
}//namespace cti

