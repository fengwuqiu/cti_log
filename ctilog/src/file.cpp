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
#include "ctilog/log/file.hpp"
#include <stdio.h>
#include <unistd.h>
#include <ulimit.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#if defined __arm__ || defined __aarch64__
#   include <linux/limits.h>
#endif
#include "ctilog/log/errno.hpp"
#include "ctilog/log/exception.hpp"
namespace cti {
namespace log
{
/**
 * 得到缓冲区里有多少字节要被读取
 * @param fd file descriptor
 * @return >= 0 (byte) when success, else -errno
 */
ssize_t AvailableByte(int const fd) noexcept
{
    /* FIONREAD 得到缓冲区里有多少字节要被读取 */
    long bytes = 0;
    if (::ioctl(fd, FIONREAD, &bytes) == -1) {
        int const e = errno;
        std::cout << __func__ << " ioctl FIONREAD fail " << strerror(e) << __FILE__ << "+"
            << __LINE__ << ".\n";
        if (e) {
            return -e;
        } else {
            return -1;
        }
    }
    return bytes;
}
/// 获取当前进程最多可以打开的文件数量
long GetMaxOpenFiles()
{
    long m = 0;
    return ::ulimit(4, m);
}
/**
 * @brief get file size
 * @param file filename
 * @return >= 0: file size when successfully else -errno
 */
ssize_t GetFileSize(std::string const& file)
{
    struct stat s;
    s.st_size = 0;
    int const ret = ::stat(file.c_str(), &s);
    if (-1 == ret) {
        int const e = errno;
        if (e) {
            return -e;
        } else {
            return -1;
        }
    }
    return s.st_size;
}
/**
 * Get file mode (文件权限)
 * @param filename the file to get mode
 * @return file mode
 * @throw Exception when fail
 */
mode_t GetFileMode(std::string const& filename)
{
    struct stat s;
    s.st_mode = 0;
    int e = ::stat(filename.c_str(), &s);
    if (e < 0) {
        e = errno;
        throw Exception("stat fail", e ? -e : -EPERM, __FILE__, __LINE__);
    }
    return s.st_mode;
}
/**
 * @brief check whether file exists
 * @param file filename
 * @return 1 when file exists, 0 when not, < 0 when error
 */
int IsExists(std::string const& file)
{
    ssize_t const sz = GetFileSize(file);
    if (sz >= 0) {
        return 1;
    } else if (-ENOENT == sz) {
        return 0;
    } else {
        return sz;
    }
}
/**
 * 获取当前路径 get current working directory
 * @return current working directory when success
 * @throw Exception when fail
 * @sa getcwd
 */
std::string GetCurrentWorkDirectory()
{
    constexpr size_t n = PATH_MAX + sizeof(void*);
    char buf[n];
    if (!::getcwd(buf, PATH_MAX)) {
        int const e = errno;
        throw Exception("getcwd fail", e ? -e : -EPERM, __FILE__, __LINE__);
    }
    buf[PATH_MAX] = 0;
    return buf;
}
/**
 * 创建符号链接 (软链接)
 * @param directory
 *   如果非空则在这个文件夹下创建符号链接, 否则 @a filename 和
 *   @a symname 应该为完整路径或者表示在当前文件夹下
 * @param filename 源文件名
 * @param symname 符号文件名
 * @throw Exception when fail
 * @sa symlink
 */
void CreateSymlink(
    std::string const& directory,
    std::string const& filename,
    std::string const& symname)
{
    bool const cd = !directory.empty();
    std::string oldPwd;
    // get oldPwd when need
    if (cd) {
        try {
            oldPwd = GetCurrentWorkDirectory();
        } catch(Exception const& e) {
            throw Exception(
                "GetCurrentWorkDirectory fail " + e.getMsg(),
                e.getCode(), __FILE__, __LINE__);
        }
        int e = ::chdir(directory.c_str());
        if (e < 0) {
            e = errno;
            throw Exception(
                "chdir fail", e ? -e : -EPERM, __FILE__, __LINE__);
        }
    }
    int const ret = ::symlink(filename.c_str(), symname.c_str());
    int const e = errno;
    if (cd) {
        // go back to previous wd
        int const ignore = ::chdir(oldPwd.c_str());
        (void)(ignore);
    }
    if (ret < 0) {
        throw Exception(
            "symlink fail", e ? -e : -EPERM, __FILE__, __LINE__);
    }
}
/**
 * remove a file or directory
 *
 * remove() deletes a name from the filesystem.
 * It calls unlink(2) for files, and rmdir(2) for directories.
 * - If the removed name was the last link to a file and no processes have the
 *   file open,
 *   the file is deleted and the space it was using is made available for
 *   reuse.
 * - If the name was the last link to a file,
 *   but any processes still have the file open,
 *   the file will remain in existence until the last file descriptor
 *   referring to it is closed.
 * - If the name referred to a symbolic link, the link is removed.
 * - If the name referred to a socket, FIFO, or device, the name is removed,
 *   but processes which have the object open may continue to use it.
 * @param pathname filename or directory path to remove
 * @return On success, zero is returned. On error, -errno is returned
 * @sa remove
 */
int RemoveFiles(std::string const& pathname)
{
    int e = ::remove(pathname.c_str());
    if (e < 0) {
        e = errno;
        return e ? -e : -EPERM;
    }
    return 0;
}
int RemoveFile(std::string const& pathname)
{
    int e = ::unlink(pathname.c_str());
    if (e < 0) {
        e = errno;
        return e ? -e : -EPERM;
    }
    return 0;
}
int RemoveFileBySystem(std::string const& pathname)
{
#   ifndef _WIN32
    std::string const s("rm -f " + pathname);
    int e = ::system(s.c_str());
    if (e) {
        e = errno;
        return e ? -e : -EPERM;
    }
    return 0;
#   else
    return ::system(("del " + pathname).c_str());
#   endif
}
/**
 * Create directories
 * @return 0 when success else -errno
 */
int MkDirs(char const* path, mode_t const mode) noexcept
{
    if (!path || ::strlen(path) <= 0) {
        return  -EINVAL;
    }
    {
        int64_t l = ::strlen(path);
        std::vector<char> buf0(l + 1 + sizeof(long));
        char* const buf = buf0.data();
        ::memcpy(buf, path, l + 1);
        if ('/' != buf[l - 1]) {
            buf[l] = '/';
            buf[l + 1] = '\0';
        }
        ++l;
        /* Mkdirs */
        for (int64_t i = 0; i < l; ++i) {
            if ('/' == buf[i] && (i > 0)) {
                buf[i] = '\0';
                if (!IsExists(buf)) {
                    if (::mkdir(buf, mode)) {
                        int ret = errno;
                        if (!ret) {
                            ret = EPERM;
                        }
                        return -ret;
                    }
                }
                buf[i] = '/';
            }
        }
    }
    return 0;// OK
}
int64_t Write2Stream(
    void const* const buf,
    uint32_t const shouldWrite,
    FILE* const stream,
    uint32_t const perWriteBytes) noexcept
{
    if ((!buf) || (!stream)) {
        return -EINVAL;
    }
    int en;
    {
        // Write
        uint8_t const* const buffer = reinterpret_cast<uint8_t const*>(buf);
        size_t w, tow;
        int64_t wt = 0;
        do {
            if ((shouldWrite - wt) >= perWriteBytes) {
                tow = perWriteBytes;
                w = ::fwrite(buffer + wt, 1, tow, stream);
            } else if ((shouldWrite - wt) > 0) {
                tow = shouldWrite - wt;
                w = fwrite(buffer + wt, 1, tow, stream);
            } else {
                break;
            }
            if (w != tow) {
                en = errno;
                goto write_fail;
            }
            wt += w;
        } while((wt < static_cast<int64_t>(shouldWrite)) && (w > 0));
        return wt;
    }
write_fail:
    if (0 != en) {
        return -en;
    } else {
        return -1;
    }
}
/// Default ctor, 使用默认文件访问模式
FileOpenConfig::FileOpenConfig(): flags(0) {}
/**
 * 由文件访问模式 @a access 构造 FileOpenFlags
 * @param access 文件访问模式
 * @sa PosixFileAccessMode
 */
FileOpenConfig::FileOpenConfig(PosixFileAccessMode const& access): flags(
    static_cast<int>(access)) {}
FileOpenConfig::FileOpenConfig(
    SimplifiedFileOpenFlag const& openFlag,
    PosixFileModes const* const fileMode): flags(0)
{
    switch (openFlag) {
    case SimplifiedFileOpenFlag::Append:
        this->flags |= static_cast<int>(PosixFileOpenFlag::Append);
        break;
    case SimplifiedFileOpenFlag::Truncate:
        this->flags |= static_cast<int>(PosixFileOpenFlag::Trunc);
        break;
    case SimplifiedFileOpenFlag::Directory:
        this->flags |= static_cast<int>(PosixFileOpenFlag::Directory);
        break;
    case SimplifiedFileOpenFlag::Create: {
        if (!fileMode) {
            throw Exception("::Create but fileMode is nullopt", -EINVAL,
                __FILE__, __LINE__);
        }
        this->flags = static_cast<int>(PosixFileCreationFlag::Creat) |
            static_cast<int>(PosixFileAccessMode::WriteOnly) |
            static_cast<int>(PosixFileCreationFlag::Trunc);
        this->mode.reset(new PosixFileModes(*fileMode));
    } break;
    default: break;
    }
}
FileOpenConfig::FileOpenConfig(
    PosixFileAccessMode const& access,
    SimplifiedFileOpenFlag const& openFlag,
    PosixFileModes const* const fileMode): flags(static_cast<int>(access))
{
    switch (openFlag) {
    case SimplifiedFileOpenFlag::Append:
        this->flags |= static_cast<int>(PosixFileOpenFlag::Append);
        break;
    case SimplifiedFileOpenFlag::Truncate:
        this->flags |= static_cast<int>(PosixFileOpenFlag::Trunc);
        break;
    case SimplifiedFileOpenFlag::Directory:
        this->flags |= static_cast<int>(PosixFileOpenFlag::Directory);
        break;
    case SimplifiedFileOpenFlag::Create: {
        if (!fileMode) {
            throw Exception("::Create but fileMode is nullopt", -EINVAL,
                __FILE__, __LINE__);
        }
        this->flags = static_cast<int>(PosixFileCreationFlag::Creat) |
            static_cast<int>(PosixFileAccessMode::WriteOnly) |
            static_cast<int>(PosixFileCreationFlag::Trunc);
        this->mode.reset(new PosixFileModes(*fileMode));
    } break;
    default: break;
    }
}
File::File(std::string const& filename): filename(filename), openConfig() {}
/// Close file when dtor
File::~File() { this->close(); }
void File::setFilename(std::string const& filename) noexcept
{
    BoostScopedWriteLock writeLock(this->filenameRwlock);
    /* close old when need */
    if (this->filename != filename) {
        this->close();
    }
    /* update filename after close */
    this->filename = filename;
}
/**
 * @return
 * - 1 when is directory
 * - 0 when not directory
 * - -errno when error
 */
int File::isDirectory() const noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    struct stat filestat;
    filestat.st_mode = 0;
    int ret;
    if (this->fd >= 0) {
        ret = ::fstat(this->fd, &filestat);
    } else {
        ret = ::stat(this->filename.c_str(), &filestat);
    }
    if (ret < 0) {
        int const e = errno;
        if (!e) {
            return -GetErrno(Errno::StatError);
        } else {
            return -GetErrno(e);
        }
    }
    return S_ISDIR(filestat.st_mode);
}
bool File::isOpen() const noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    return (this->fd >= 0);
}
/// get total file size
ssize_t File::size() const noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    return this->__size();
}
ssize_t File::__size() const
{
    struct stat filestat;
    filestat.st_size = 0;
    int ret;
    if (this->fd >= 0) {
        ret = ::fstat(this->fd, &filestat);
    } else {
        ret = ::stat(this->filename.c_str(), &filestat);
    }
    if (ret < 0) {
        int const e = errno;
        if (!e) {
            return -GetErrno(Errno::StatError);
        } else {
            return -GetErrno(e);
        }
    }
    if (filestat.st_size >= 0) {
        return filestat.st_size;
    } else {
        return -GetErrno(Errno::StatError);
    }
}
/// get current io position
ssize_t File::ioPostion() const noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    if (this->fd < 0) {
        return -GetErrno(Errno::NotOpen);
    }
    {
        /* exclusive io */
        std::lock_guard<std::mutex> lock(this->ioMutex);
        off_t const pos = ::lseek(this->fd, 0, SEEK_CUR);
        if (-1 == static_cast<ssize_t>(pos)) {
            int const e = errno;
            if (!e) {
                return -GetErrno(Errno::SeekError);
            } else {
                return -GetErrno(e);
            }
        }
        return pos;
    }
}
/// get current rest io size
ssize_t File::ioRestPostion() const noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    if (this->fd < 0) {
        return -GetErrno(Errno::NotOpen);
    }
    {
        /* exclusive io */
        std::lock_guard<std::mutex> lock(this->ioMutex);
        off_t const pos = ::lseek(this->fd, 0, SEEK_CUR);
        if (-1 == static_cast<ssize_t>(pos)) {
            int const e = errno;
            if (!e) {
                return -GetErrno(Errno::SeekError);
            } else {
                return -GetErrno(e);
            }
        }
        ssize_t const total = this->__size();
        if (total < 0) {
            return total;
        }
        return total - pos;
    }
}
/// Set fd postion to begin, return >= 0 when succes else -errno
int File::jump2Begin() noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    if (this->fd < 0) {
        return -GetErrno(Errno::NotOpen);
    }
    {
        // Exclusive io
        std::lock_guard<std::mutex> lock(this->ioMutex);
        off_t const pos = ::lseek(this->fd, 0, SEEK_SET);
        if (-1 == static_cast<ssize_t>(pos)) {
            int const e = errno;
            if (!e) {
                return -GetErrno(Errno::SeekError);
            } else {
                return -GetErrno(e);
            }
        }
        return 0;
    }
}
/**
 * Set fd postion to current + @a offset
 * @return >= 0 when succes else -errno
 */
int File::jump2Offset(long const offset) noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    if (this->fd < 0) {
        return -GetErrno(Errno::NotOpen);
    }
    {
        // Exclusive io
        std::lock_guard<std::mutex> lock(this->ioMutex);
        off_t const pos = ::lseek(this->fd, offset, SEEK_CUR);
        if (-1 == static_cast<ssize_t>(pos)) {
            int const e = errno;
            if (!e) {
                return -GetErrno(Errno::SeekError);
            } else {
                return -GetErrno(e);
            }
        }
        return 0;
    }
}
int File::rjump2Offset(long const offset) noexcept
{
    BoostScopedReadLock readLock(this->fdRwlock);
    if (this->fd < 0) {
        return -GetErrno(Errno::NotOpen);
    }
    {
        // Exclusive io
        std::lock_guard<std::mutex> lock(this->ioMutex);
        off_t const pos = ::lseek(this->fd, offset, SEEK_END);
        if (-1 == static_cast<ssize_t>(pos)) {
            int const e = errno;
            if (!e) {
                return -GetErrno(Errno::SeekError);
            } else {
                return -GetErrno(e);
            }
        }
        return 0;
    }
}
/**
 * Open this file use @a openConfig
 * @param openConfig file open flag and create mode(when need)
 * @return >= 0 when succes else -errno
 */
int File::open(FileOpenConfig const& openConfig)
{
    BoostScopedWriteLock writeLock(this->fdRwlock);
    if (this->__isOpen()) {
        return -GetErrno(Errno::AlreadyOpen);
    }
    if (this->filename.length() <= 0) {
        return -GetErrno(Errno::InvalidFilename);
    }
    int fd;
    int const openFlag = openConfig.getFlags();
    auto const fileMode = openConfig.getMode();
    if (!fileMode) {
        fd = File::open(this->filename, openFlag);
    } else {
        fd = File::open(this->filename, openFlag, mode_t(int(*fileMode)));
    }
    if (fd < 0) {
        return fd;
    }
    {
        std::lock_guard<std::mutex> lock(this->ioMutex);
        this->fd = fd;
    }
    this->openConfig = openConfig;
    return GetErrno(Errno::Ok);
}
int File::open(std::string const& filename, int const flag)
{
    /* check create flag */
    if (flag & static_cast<int>(PosixFileOpenFlag::Creat)) {
        return -GetErrno(Errno::InvalidParam);
    }
    int const ret = ::open(filename.c_str(), flag);
    if (ret >= 0) {
        return ret;
    }
    int const e = abs(errno);
    if (!e) {
        return -GetErrno(Errno::OpenError);
    } else {
        return -GetErrno(e);
    }
}
int File::open(std::string const& filename, int const flag, mode_t const mode)
{
    int const ret = ::open(filename.c_str(), flag, mode);
    if (ret >= 0) {
        return ret;
    }
    int const e = abs(errno);
    if (!e) {
        return -GetErrno(Errno::OpenError);
    } else {
        return -GetErrno(e);
    }
}
/// Close this file
void File::close() noexcept
{
    BoostScopedWriteLock writeLock(this->fdRwlock);
    if (this->fd >= 0) {
        std::lock_guard<std::mutex> lock(this->ioMutex);
        ::close(this->fd);
        this->fd = -1;
    }
}
/**
 * read once @a size byes
 * @param size if >= 0 read @a size byes, if < 0 and real file rest size
 * <= kFileIoUpperBound read all, else if size < 0 and real file rest size
 * > kFileIoUpperBound read fail please use another read!
 * @param checkavail true to check available else false
 * @returns { code, readdata }
 */
std::tuple<int, boost::shared_ptr<std::vector<uint8_t> > > File::read(
    ssize_t const size, bool checkavail) noexcept
{
    using RType = std::tuple<int, boost::shared_ptr<std::vector<uint8_t> > >;
    /* chk busy */
    if (this->ioBusy) {
        std::cout << __func__ << " busy " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return RType{ -GetErrno(Errno::Busy), nullptr };
    }
    IoBusyGuard iobusy(this->ioBusy);/* make io busy */
    (void)(iobusy);
    int ret, initfd;
    boost::shared_ptr<std::vector<uint8_t> > buffer = nullptr;
    std::string initfilename;
    {
        // get primary
        BoostScopedReadLock readLock(this->fdRwlock);
        initfd = this->fd;
        initfilename = this->filename;
    }
    /* check fd i.e. chk open */
    if (initfd < 0) {
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
    size_t maxRead;
    if (checkavail || size < 0) {
        ssize_t const fileSize = this->__size();
        if (fileSize < 0) {
            ret = fileSize;
            goto end;
        }
        if ((size < 0) || (size > fileSize)) {
            maxRead = fileSize;
        } else {
            maxRead = size;
        }
    } else {
        maxRead = size;
    }
    if (maxRead > kFileIoUpperBound) {
        ret = -GetErrno(Errno::InvalidParam);
        goto end;
    }
    try {
        buffer.reset(new std::vector<uint8_t>());
        buffer->reserve(maxRead);
    } catch (std::exception const& e) {
        std::cout << __func__ << " alloc buffer fail " << e.what() << __FILE__ << "+"
            << __LINE__ << ".\n";
        ret = -GetErrno(Errno::NoMem);
        goto end;
    }
    ssize_t didread;
    int fd;
    std::string filename;
    {
        /* get init incase changed */
        BoostScopedReadLock readLock(this->fdRwlock);
        fd = this->fd;
        filename = this->filename;
    }
    /* check fd .. changed */
    if ((fd != initfd) || (filename != initfilename)) {
        std::cout << __func__ << " file changed " << __FILE__ << "+"
            << __LINE__ << ".\n";
        ret = -GetErrno(Errno::FileChanged);
        buffer = nullptr;
        goto end;
    }
    buffer->resize(maxRead);
    {
        /* lock to read! (exclusive io) */
        std::lock_guard<std::mutex> lock(this->ioMutex);
        didread = ::read(fd, buffer->data(), maxRead);
    }
    /* chk if fail */
    if (didread < 0) {
        int const e = errno;
        std::cout << __func__ << " read fail " << e << __FILE__ << "+"
            << __LINE__ << ".\n";
        if (!e) {
            ret = -GetErrno(Errno::ReadError);
        } else {
            ret = -GetErrno(e);
        }
        buffer = nullptr;
        goto end;
    }
    /* success */
    buffer->resize(didread);
    buffer->shrink_to_fit();
    ret = 0;
    }
end:
    return RType{ ret, buffer };
}
std::tuple<int32_t, uint64_t> File::traverse(
    std::function<bool(uint8_t const* const data, uint32_t const size)>
        didRead,
    uint64_t const eachRead0,
    int64_t const limit) noexcept
{
    using RT = std::tuple<int, uint64_t>;
    // Save total traversed bytes
    uint64_t total = 0;
    // chk busy
    if (this->ioBusy) {
        std::cerr << __func__ << ": file io busy, " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return RT{ -GetErrno(Errno::Busy), total };
    }
    // Make io busy
    IoBusyGuard iobusy(this->ioBusy);
    int ret=0;
    int primaryFd;
    std::string primaryFilename;
    {
        // Get primary fd and filename, incase changed
        BoostScopedReadLock readLock(this->fdRwlock);
        primaryFd = this->fd;
        primaryFilename = this->filename;
    }
    // Check fd i.e. chk open
    if (primaryFd < 0) {
        std::cerr << __func__ << ": file not open, " << __FILE__ << "+"
            << __LINE__ << ".\n";
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
        int64_t const fileSize = this->__size();
        if (fileSize < 0) {
            ret = int32_t(fileSize);
            goto end;
        }
        uint64_t maxRead;
        if ((limit < 0) || (limit > fileSize)) {
            maxRead = fileSize;
        } else {
            maxRead = limit;
        }
        uint64_t eachRead;
        if (eachRead0 <= 0) {
            eachRead = kFileIoUpperBound;
        } else if (eachRead0 > kMaxFileRdMem) {
            eachRead = kMaxFileRdMem;
        } else {
            eachRead = eachRead0;
        }
        if (eachRead > maxRead) {
            eachRead = maxRead;
        }
        uint8_t* buffer = nullptr;
        int64_t offset, paOffset, length;
        int fd;
        std::string filename;
        do {
            {
                // Get primary fd and filename again, incase changed
                BoostScopedReadLock readLock(this->fdRwlock);
                fd = this->fd;
                filename = this->filename;
            }
            // Check fd .. changed
            if ((fd != primaryFd) || (filename != primaryFilename)) {
                std::cerr << __func__ << ": file changed, " << __FILE__ << "+"
                    << __LINE__ << ".\n";
                ret = -GetErrno(Errno::FileChanged);
                goto end;
            }
            offset = total;
            // Offset for mmap() must be page aligned
            paOffset = offset & ~(::sysconf(_SC_PAGE_SIZE) - 1);
            if (eachRead <= (maxRead - total)) {
                length = eachRead;
            } else {
                length = maxRead - total;
            }
            {
                // Lock io to read! (exclusive io) */
                std::lock_guard<std::mutex> lock(this->ioMutex);
                buffer = reinterpret_cast<uint8_t*>(::mmap(
                    nullptr,
                    length + offset - paOffset,
                    PROT_READ,
                    MAP_PRIVATE,
                    fd,
                    paOffset));
            }
            // Chk if mmap fail
            if (MAP_FAILED == buffer) {
                int const e = errno;
                std::cerr << __func__ << ": mmap fail: " << e << ", "
                    << __FILE__ << "+" << __LINE__ << ".\n";
                if (!e) {
                    ret = -GetErrno(Errno::ReadError);
                } else {
                    ret = -GetErrno(e);
                }
                goto end;
            }
            // Success and accumulate total
            total += length;
            // Success and callback
            if (didRead(buffer, length)) {
                ::munmap(buffer, length + offset - paOffset);
                std::cout << __func__ << ": cancelled, " << __FILE__ << "+"
                    << __LINE__ << ".\n";
                ret = GetErrno(Errno::Cancelled);
                goto end;
            }
            ::munmap(buffer, length + offset - paOffset);
            // Check end
            if (0 == length) {
                ret = 0;
                break;
            }
        } while (total < maxRead);
    }
end:
    return RT{ ret, total };
}
std::tuple<int, size_t> File::read(
    std::function<bool(uint8_t const* const data, size_t const size)>
        didRead,
    size_t const eachRead0,
    ssize_t const limit) noexcept
{
    size_t total = 0;
    /* chk busy */
    if (this->ioBusy) {
        std::cout << __func__ << " busy " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::Busy), total);
    }
    IoBusyGuard iobusy(this->ioBusy);/* make io busy */
    (void)(iobusy);
    int ret, initfd;
    std::string initfilename;
    {
        /* get init */
        BoostScopedReadLock readLock(this->fdRwlock);
        initfd = this->fd;
        initfilename = this->filename;
    }
    /* check fd i.e. chk open */
    if (initfd < 0) {
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
    ssize_t const fileSize = this->__size();
    if (fileSize < 0) {
        ret = fileSize;
        goto end;
    }
    size_t maxRead;
    if ((limit < 0) || (limit > fileSize)) {
        maxRead = fileSize;
    } else {
        maxRead = limit;
    }
    size_t eachRead;
    if (eachRead0 <= 0) {
        eachRead = kFileIoUpperBound;
    } else if (eachRead0 > kMaxFileRdMem) {
        eachRead = kMaxFileRdMem;
    } else {
        eachRead = eachRead0;
    }
    if (eachRead > maxRead) {
        eachRead = maxRead;
    }
    boost::shared_ptr<std::vector<uint8_t> > buffer;
    try {
        buffer.reset(new std::vector<uint8_t>());
        buffer->reserve(eachRead);
    } catch (std::exception const& e) {
        std::cout << __func__ << " alloc buffer fail " << e.what() << __FILE__ << "+"
            << __LINE__ << ".\n";
        ret = -GetErrno(Errno::NoMem);
        goto end;
    }
    ssize_t didread;
    int fd;
    std::string filename;
    do {
        {
            /* get init incase changed */
            BoostScopedReadLock readLock(this->fdRwlock);
            fd = this->fd;
            filename = this->filename;
        }
        /* check fd .. changed */
        if ((fd != initfd) || (filename != initfilename)) {
            std::cout << __func__ << " file changed " << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = -GetErrno(Errno::FileChanged);
            goto end;
        }
        buffer->resize(eachRead);
        {
            /* lock to read! (exclusive io) */
            std::lock_guard<std::mutex> lock(this->ioMutex);
            if (eachRead <= (maxRead - total)) {
                didread = ::read(fd, buffer->data(), eachRead);
            } else {
                didread = ::read(fd, buffer->data(), maxRead - total);
            }
        }
        /* chk if fail */
        if (didread < 0) {
            int const e = errno;
            std::cout << __func__ << " read fail " << e << __FILE__ << "+"
                << __LINE__ << ".\n";
            if (!e) {
                ret = -GetErrno(Errno::ReadError);
            } else {
                ret = -GetErrno(e);
            }
            goto end;
        }
        /* success and acc total */
        total += didread;
        /* success and callback */
        buffer->resize(didread);
        if (didRead(buffer->data(), buffer->size())) {
            std::cout << __func__ << " cancelled " << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = GetErrno(Errno::Cancelled);
            goto end;
        }
        /* check end */
        if (0 == didread) {
            ret = 0;
            break;
        }
    } while (total < maxRead);
    }
end:
    this->jump2Offset(total);
    return std::tuple<int, size_t>(ret, total);
}
std::tuple<int, size_t> File::write(
    Any const& data, ssize_t const eachWrite0, ssize_t const limit) noexcept
{
    size_t total = 0;
    if (!data) {
        std::cout << __func__ << " invallid data " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::InvalidData), total);
    }
    /* chk busy */
    if (this->ioBusy) {
        std::cout << __func__ << " busy " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::Busy), total);
    }
    IoBusyGuard iobusy(this->ioBusy);/* make io busy */
    (void)(iobusy);
    int ret, initfd;
    std::string initfilename;
    {
        /* get init */
        boost::shared_lock<boost::shared_mutex> rLock(this->fdRwlock);
        initfd = this->fd;
        initfilename = this->filename;
    }
    /* check fd i.e. chk open */
    if (initfd < 0) {
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
    void const* dataPtr;
    size_t dataSize;
    std::tie(ret, dataPtr, dataSize) = data.getDataBundle();
    if (ret < 0) {
        std::cout << __func__ << " getDataBundle fail " << ret << __FILE__ << "+"
            << __LINE__ << ".\n";
        goto end;
    }
    size_t maxWrite;
    if ((limit < 0) || (static_cast<size_t>(limit) > dataSize)) {
        maxWrite = dataSize;
    } else {
        maxWrite = limit;
    }
    size_t eachWrite;
    if (0 == eachWrite0) {
        eachWrite = kFileIoUpperBound;
    } else if (eachWrite0 < 0) {
        eachWrite = maxWrite;
    } else {
        eachWrite = eachWrite0;
    }
    if (eachWrite > maxWrite) {
        eachWrite = maxWrite;
    }
    ssize_t didwrite;
    int fd;
    std::string filename;
    do {
        {
            /* get init incase changed */
            boost::shared_lock<boost::shared_mutex> rLock(this->fdRwlock);
            fd = this->fd;
            filename = this->filename;
        }
        /* check fd .. changed */
        if ((fd != initfd) || (filename != initfilename)) {
            std::cout << __func__ << " file changed " << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = -GetErrno(Errno::FileChanged);
            goto end;
        }
        {
            /* lock to read! (exclusive io) */
            std::lock_guard<std::mutex> lock(this->ioMutex);
            /* write! */
            (void)(lock);
            if (eachWrite <= (maxWrite - total)) {
                didwrite = ::write(
                    fd,
                    reinterpret_cast<uint8_t const*>(dataPtr) + total,
                    eachWrite);
            } else {
                didwrite = ::write(
                    fd,
                    reinterpret_cast<uint8_t const*>(dataPtr) + total,
                    maxWrite - total);
            }
        }
        /* chk if fail */
        if (didwrite < 0) {
            int const e = errno;
            std::cout << __func__ << " write fail " << strerror(e) << __FILE__ << "+"
                << __LINE__ << ".\n";
            if (!e) {
                ret = -GetErrno(Errno::ReadError);
            } else {
                ret = -GetErrno(e);
            }
            goto end;
        }
        /* acc total */
        total += didwrite;
        /* check end */
        if (0 == didwrite) {
            ret = 0;
            break;
        }
    } while (total < maxWrite);
    }
end:
    return std::tuple<int, size_t>(ret, total);
}
std::tuple<int32_t, uint64_t> File::write(
    void const* const data,
    uint64_t const maxWrite,
    int32_t const eachWrite0) noexcept
{
    using RT = std::tuple<int32_t, uint64_t>;
    uint64_t total = 0;
    if (!data) {
        std::cerr << __func__ << ": invalid data, " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return RT{ -GetErrno(Errno::InvalidData), total };
    }
    // Chk io busy
    if (this->ioBusy) {
        std::cerr << __func__ << ": io busy, " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return RT{ -GetErrno(Errno::Busy), total };
    }
    // Make io busy
    IoBusyGuard iobusy(this->ioBusy);
    int ret;
    int primaryFd;
    std::string primaryFilename;
    {
        BoostScopedReadLock readLock(this->fdRwlock);
        primaryFd = this->fd;
        primaryFilename = this->filename;
    }
    // Check fd i.e. chk open
    if (primaryFd < 0) {
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
        uint64_t eachWrite;
        if (0 == eachWrite0) {
            eachWrite = kFileIoUpperBound;
        } else if (eachWrite0 < 0) {
            eachWrite = maxWrite;
        } else {
            eachWrite = eachWrite0;
        }
        if (eachWrite > maxWrite) {
            eachWrite = maxWrite;
        }
        int64_t didwrite;
        int fd;
        std::string filename;
        do {
            {
                BoostScopedReadLock readLock(this->fdRwlock);
                fd = this->fd;
                filename = this->filename;
            }
            // Check fd .. changed
            if ((fd != primaryFd) || (filename != primaryFilename)) {
                std::cerr << __func__ << ": file changed, " << __FILE__ << "+"
                    << __LINE__ << ".\n";
                ret = -GetErrno(Errno::FileChanged);
                goto end;
            }
            {
                // Lock io to read! (exclusive io)
                std::lock_guard<std::mutex> lock(this->ioMutex);
                // Write!
                if (eachWrite <= (maxWrite - total)) {
                    didwrite = ::write(
                        fd,
                        reinterpret_cast<uint8_t const*>(data) + total,
                        eachWrite);
                } else {
                    didwrite = ::write(
                        fd,
                        reinterpret_cast<uint8_t const*>(data) + total,
                        maxWrite - total);
                }
            }
            // Chk if fail
            if (didwrite < 0) {
                int const e = errno;
                std::cerr << __func__ << ": write fail, " << __FILE__ << "+"
                    << __LINE__ << ".\n";
                if (!e) {
                    ret = -GetErrno(Errno::ReadError);
                } else {
                    ret = -GetErrno(e);
                }
                goto end;
            }
            // Accumulate total
            total += didwrite;
            // Check end
            if (0 == didwrite) {
                ret = 0;
                break;
            }
        } while(total < maxWrite);
    }
end:
    return RT{ ret, total };
}
/**
 * write with callback when each write
 * @sa write
 */
std::tuple<int, size_t> File::write(
    std::function<bool(size_t const wrote, size_t const rest)> didWrote,
    Any const& data,
    ssize_t const eachWrite0,
    ssize_t const limit) noexcept
{
    size_t total = 0;
    if (!data) {
        std::cout << __func__ << " invalid data " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::InvalidData), total);
    }
    /* chk busy */
    if (this->ioBusy) {
        std::cout << __func__ << " busy " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::Busy), total);
    }
    IoBusyGuard iobusy(this->ioBusy);/* make io busy */
    (void)(iobusy);
    int ret, initfd;
    std::string initfilename;
    {
        /* get init */
        boost::shared_lock<boost::shared_mutex> rLock(this->fdRwlock);
        initfd = this->fd;
        initfilename = this->filename;
    }
    /* check fd i.e. chk open */
    if (initfd < 0) {
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
    void const* dataPtr;
    size_t dataSize;
    std::tie(ret, dataPtr, dataSize) = data.getDataBundle();
    if (ret < 0) {
        std::cout << __func__ << " getDataBundle fail " << ret << __FILE__ << "+"
            << __LINE__ << ".\n";
        goto end;
    }
    size_t maxWrite;
    if ((limit < 0) || (static_cast<size_t>(limit) > dataSize)) {
        maxWrite = dataSize;
    } else {
        maxWrite = limit;
    }
    size_t eachWrite;
    if (0 == eachWrite0) {
        eachWrite = kFileIoUpperBound;
    } else if (eachWrite0 < 0) {
        eachWrite = maxWrite;
    } else {
        eachWrite = eachWrite0;
    }
    if (eachWrite > maxWrite) {
        eachWrite = maxWrite;
    }
    ssize_t didwrite;
    int fd;
    std::string filename;
    do {
        {
            /* get init incase changed */
            boost::shared_lock<boost::shared_mutex> rLock(this->fdRwlock);
            fd = this->fd;
            filename = this->filename;
        }
        /* check fd .. changed */
        if ((fd != initfd) || (filename != initfilename)) {
            std::cout << __func__ << " file changed " << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = -GetErrno(Errno::FileChanged);
            goto end;
        }
        {
            /* lock to read! (exclusive io) */
            std::lock_guard<std::mutex> lock(this->ioMutex);
            /* write! */
            (void)(lock);
            if (eachWrite <= (maxWrite - total)) {
                didwrite = ::write(
                    fd,
                    reinterpret_cast<uint8_t const*>(dataPtr) + total,
                    eachWrite);
            } else {
                didwrite = ::write(
                    fd,
                    reinterpret_cast<uint8_t const*>(dataPtr) + total,
                    maxWrite - total);
            }
        }
        /* chk if fail */
        if (didwrite < 0) {
            int const e = errno;
            std::cout << __func__ << " write fail " << e << __FILE__ << "+"
                << __LINE__ << ".\n";
            if (!e) {
                ret = -GetErrno(Errno::ReadError);
            } else {
                ret = -GetErrno(e);
            }
            goto end;
        }
        /* acc total */
        total += didwrite;
        /* succes and callback */
        if (didWrote(total, maxWrite - total)) {
            std::cout << __func__ << " cancelled " << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = GetErrno(Errno::Cancelled);
            goto end;
        }
        /* check end */
        if (0 == didwrite) {
            ret = 0;
            break;
        }
    } while (total < maxWrite);
    }
end:
    return std::tuple<int, size_t>(ret, total);
}
std::tuple<int, size_t> File::write(
    std::function<bool(size_t const wrote, size_t const rest)> didWrote,
    void const* const data,
    size_t const size,
    ssize_t const eachWrite0,
    ssize_t const limit) noexcept
{
    size_t total = 0;
    if (!data) {
        std::cout << __func__ << " invalid data " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::InvalidData), total);
    }
    /* chk busy */
    if (this->ioBusy) {
        std::cout << __func__ << " busy " << __FILE__ << "+"
            << __LINE__ << ".\n";
        return std::tuple<int, size_t>(-GetErrno(Errno::Busy), total);
    }
    IoBusyGuard iobusy(this->ioBusy);/* make io busy */
    (void)(iobusy);
    int ret, initfd;
    std::string initfilename;
    {
        // get init
        BoostScopedReadLock readLock(this->fdRwlock);
        initfd = this->fd;
        initfilename = this->filename;
    }
    /* check fd i.e. chk open */
    if (initfd < 0) {
        ret = -GetErrno(Errno::NotOpen);
        goto end;
    }
    {
    size_t maxWrite;
    if ((limit < 0) || (static_cast<size_t>(limit) > size)) {
        maxWrite = size;
    } else {
        maxWrite = limit;
    }
    size_t eachWrite;
    if (0 == eachWrite0) {
        eachWrite = kFileIoUpperBound;
    } else if (eachWrite0 < 0) {
        eachWrite = maxWrite;
    } else {
        eachWrite = eachWrite0;
    }
    if (eachWrite > maxWrite) {
        eachWrite = maxWrite;
    }
    ssize_t didwrite;
    int fd;
    std::string filename;
    do {
        {
            // get init incase changed
            BoostScopedReadLock readLock(this->fdRwlock);
            fd = this->fd;
            filename = this->filename;
        }
        /* check fd .. changed */
        if ((fd != initfd) || (filename != initfilename)) {
            std::cout << __func__ << "file changed" << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = -GetErrno(Errno::FileChanged);
            goto end;
        }
        {
            /* lock to read! (exclusive io) */
            std::lock_guard<std::mutex> lock(this->ioMutex);
            /* write! */
            if (eachWrite <= (maxWrite - total)) {
                didwrite = ::write(
                    fd,
                    reinterpret_cast<uint8_t const*>(data) + total,
                    eachWrite);
            } else {
                didwrite = ::write(
                    fd,
                    reinterpret_cast<uint8_t const*>(data) + total,
                    maxWrite - total);
            }
        }
        /* chk if fail */
        if (didwrite < 0) {
            int const e = errno;
            std::cout << __func__ << "write fail " << e << __FILE__ << "+"
                << __LINE__ << ".\n";
            if (!e) {
                ret = -GetErrno(Errno::ReadError);
            } else {
                ret = -GetErrno(e);
            }
            goto end;
        }
        /* acc total */
        total += didwrite;
        /* succes and callback */
        if (didWrote(total, maxWrite - total)) {
            std::cout << __func__ << ": cancelled, " << __FILE__ << "+"
                << __LINE__ << ".\n";
            ret = GetErrno(Errno::Cancelled);
            goto end;
        }
        /* check end */
        if (0 == didwrite) {
            ret = 0;
            break;
        }
    } while (total < maxWrite);
    }
end:
    return std::tuple<int, size_t>(ret, total);
}
#if 0
static int64_t ConsumeStream(
    FILE* const stream,
    std::function<bool(void const* const data, uint32_t const size)> const&
        freadConsumer,
    int64_t maxRead = -1)
{
    if ((!stream) || (!freadConsumer)) {
        return -EINVAL;
    }
    uint64_t rd;
    /* Read */
    {
        int32_t r;
        std::vector<uint8_t> buf(kPerReadBytes);
        rd = 0;
        do {
            ::memset(buf.data(), 0, buf.size());
            if ((maxRead < 0) || (maxRead - rd) >= kPerReadBytes) {
                r = ::fread(buf.data(), 1, kPerReadBytes, stream);
            } else if ((maxRead - rd) > 0) {
                r = ::fread(buf.data(), 1, maxRead - rd, stream);
            } else {
                break;
            }
            rd += r;
            if (r > 0) {
                if (freadConsumer(buf.data(), r)) {
                    break;
                }
            }
            if (::feof(stream)) {
                break;
            }
        } while(r > 0);
    }
    return rd;// OK
}
#endif
}//namespace log
}//namespace cti
