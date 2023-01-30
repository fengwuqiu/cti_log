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
#ifndef _LARGEFILE64_SOURCE
#   define _LARGEFILE64_SOURCE 1
#endif
#ifdef _FILE_OFFSET_BITS
#   undef _FILE_OFFSET_BITS
#endif
#ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <mutex>
#include "boost/optional.hpp"
#include "boost/function.hpp"
#include "boost/thread/shared_mutex.hpp"
#include "any.hpp"
#include "scopedrwlock.hpp"
#include "flags.hpp"
namespace cti {
namespace log
{
// Some file related specs.
static_assert(sizeof(long) <= 8, "support 64 or 32");
/**
 * kFileIoUpperBound: Small file io upper bound bytes 1 GB / 512 MB
 * Primarily used in File
 * @sa File
 */
constexpr uint32_t kFileIoUpperBound = sizeof(long) * 128 * 1024 * 1024;
/// Max file read memory 4 GB / 2GB
constexpr uint64_t kMaxFileRdMem = sizeof(long) * 512 * 1024 * 1024;
/// NOTE kPerReadBytes maybe small
constexpr uint32_t kPerReadBytes = 4096 * sizeof(long);
constexpr uint32_t kPerWriteBytes = 4096 * sizeof(long);
/// 128 MB / 64 MB
constexpr uint32_t kBigPerReadBytes = sizeof(long) * 16 * 1024 * 1024;
/**
 * 得到缓冲区里有多少字节要被读取
 * @param fd file descriptor
 * @return >= 0 (byte) when success, else -errno
 */
extern ssize_t AvailableByte(int const fd) noexcept;
/// 获取当前进程最多可以打开的文件数量
extern long GetMaxOpenFiles();
/*extern int SetMaxOpenFiles(size_t const max);*/
/**
 * @brief get file size
 * @param file filename
 * @return >= 0: file size when successfully else -errno
 */
extern ssize_t GetFileSize(std::string const& file);
/**
 * Get file mode (文件权限)
 * @param filename the file to get mode
 * @return file mode
 * @throw Exception when fail
 */
extern mode_t GetFileMode(std::string const& filename);
/**
 * @brief check whether file exists
 * @param file filename
 * @return 1 when file exists, 0 when not, < 0 when error
 */
extern int IsExists(std::string const& file);
/**
 * 获取当前路径 get current working directory
 * @return current working directory when success
 * @throw Exception when fail
 * @sa getcwd
 */
extern std::string GetCurrentWorkDirectory();
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
extern void CreateSymlink(
    std::string const& directory,
    std::string const& filename,
    std::string const& symname);
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
extern int RemoveFiles(std::string const& pathname);
extern int RemoveFile(std::string const& pathname);
extern int RemoveFileBySystem(std::string const& pathname);
/**
 * Create directories
 * @return 0 when success else -errno
 */
extern int MkDirs(char const* path, mode_t const mode = 0755) noexcept;
/**
 * Write @a buf with to @a stream @a size bytes
 * @note @a buf should has @a size bytes accessable data when @a buf not nil
 * @return bytes did write when success else -errno
 */
extern int64_t Write2Stream(
    void const* const buf,
    uint32_t const size,
    FILE* const stream,
    uint32_t const perWriteBytes = kBigPerReadBytes) noexcept;
/**
 * @enum PosixFileAccessMode
 * File access mode, required when open a file, default O_RDONLY(0)
 * @note NO |
 */
enum class PosixFileAccessMode {
    ReadOnly  = O_RDONLY,
    WriteOnly = O_WRONLY,
    ReadWrite = O_RDWR,
};
/**
 * @enum PosixFileCreationFlag
 * 文件创建 flag
 * @sa
 * - PosixFileOpenFlag
 * - FileOpenConfig
 */
enum class PosixFileCreationFlag {
    CloExec   = O_CLOEXEC,
    Creat     = O_CREAT,
    Directory = O_DIRECTORY,
    Excl      = O_EXCL,
    NoCTty    = O_NOCTTY,
    NoFollow  = O_NOFOLLOW,
    TmpFile   = O_TMPFILE,
    Trunc     = O_TRUNC,
};
/**
 * @enum PosixFileOpenFlag
 * 打开文件使用的 flag
 * @sa
 * - PosixFileCreationFlag
 * - PosixFileOpenFlags
 */
enum class PosixFileOpenFlag {
    CloExec   = int(PosixFileCreationFlag::CloExec),
    Creat     = int(PosixFileCreationFlag::Creat),
    Excl      = int(PosixFileCreationFlag::Excl),
    NoCTty    = int(PosixFileCreationFlag::NoCTty),
    NoFollow  = int(PosixFileCreationFlag::NoFollow),
    TmpFile   = int(PosixFileCreationFlag::TmpFile),
    Trunc     = int(PosixFileCreationFlag::Trunc),
    Append    = O_APPEND,
    Async     = O_ASYNC,
    Direct    = O_DIRECT,
    Directory = O_DIRECTORY,
    Dsync     = O_DSYNC,
    LargeFile = O_LARGEFILE,
    NoATime   = O_NOATIME,
    NonBlock  = O_NONBLOCK,
    Path      = O_PATH,
    NDelay    = O_NDELAY,
    Sync      = O_SYNC,
};
/**
 * @typedef PosixFileOpenFlags
 * 可以使用 | 的 PosixFileOpenFlag
 */
typedef Flags<PosixFileOpenFlag> PosixFileOpenFlags;
/**
 * @struct SimplifiedFileOpenFlag
 * 简化的文件打开 flag
 * @note NO |
 * @sa
 * - PosixFileOpenFlag
 * - PosixFileCreationFlag
 * - PosixFileAccessMode
 */
enum class SimplifiedFileOpenFlag {
    Append, ///< open flags: + O_APPEND
    Truncate, ///< open flags: + O_TRUNC
    Directory, ///< open flags: + O_DIRECTORY
    Create, ///< open flags: = O_CREAT | O_WRONLY | O_TRUNC
};
/**
 * @enum PosixFileMode
 * 文件创建权限
 * @sa PosixFileModes
 */
enum class PosixFileMode: mode_t {
    /// 00700 user (file owner) has read, write, and execute permission
    Rwxu = S_IRWXU,
    Rusr = S_IRUSR, ///< 00400 user has read permission
    Wusr = S_IWUSR, ///< 00200 user has write permission
    Xusr = S_IXUSR, ///< 00100 user has execute permission
    Rwxg = S_IRWXG, ///< 00070 group has read, write, and execute permission
    Rgrp = S_IRGRP, ///< 00040 group has read permission
    Wgrp = S_IWGRP, ///< 00020 group has write permission
    Xgrp = S_IXGRP, ///< 00010 group has execute permission
    Rwxo = S_IRWXO, ///< 00007 others have read, write, and execute permission
    Roth = S_IROTH, ///< 00004 others have read permission
    Woth = S_IWOTH, ///< 00002 others have write permission
    Xoth = S_IXOTH, ///< 00001 others have execute permission
    /* According to POSIX, the effect when other bits are set in mode is
     * unspecified.  On Linux, the following bits are also honored in mode: */
    Suid = S_ISUID, ///< 0004000 set-user-ID bit
    Sgid = S_ISGID, ///< 0002000 set-group-ID bit (see stat(2))
    Svtx = S_ISVTX, ///< 0001000 sticky bit (see stat(2))
};
/**
 * @typedef PosixFileModes
 * 可以使用 | 的 PosixFileMode
 */
typedef Flags<PosixFileMode> PosixFileModes;
/**
 * @struct FileOpenConfig
 * 文件打开 flag 和 mode(创建时需要)
 */
struct FileOpenConfig {
    /// Default ctor, 使用默认文件访问模式
    FileOpenConfig();
    /**
     * 由文件访问模式 @a access 构造 FileOpenConfig
     * @param access 文件访问模式
     * @sa PosixFileAccessMode
     */
    FileOpenConfig(PosixFileAccessMode const& access);
    /**
     * 由默认文件访问模式和简化的文件打开 flag @a openFlag 等构造 FileOpenConfig
     * @param openFlag 简化的文件打开 flag
     * @param fileMode
     *   默认 nullopt, 如果 @a openMode 是 FileOpenMode::Create
     *   @a fileMode 不能为 nullopt
     *
     * @note if FileOpenMode::Create provided @a access and other mode will
     * be ignore
     *
     * @throw Exception 当 @a openMode 是 FileOpenMode::Create 但是 @a fileMode
     * 是 nullopt
     * @sa
     * - PosixFileAccessMode
     * - SimplifiedFileOpenMode
     * - FileModes
     */
    FileOpenConfig(
        SimplifiedFileOpenFlag const& openFlag,
        PosixFileModes const* const fileMode = nullptr);
    /**
     * 由默认文件访问模式和简化的文件打开 flag @a openFlag 等构造 FileOpenConfig
     * @param access 文件访问模式
     * @param openFlag 简化的文件打开 flag
     * @param fileMode
     *   默认 nullopt, 如果 @a openMode 是 FileOpenMode::Create
     *   @a fileMode 不能为 nullopt
     *
     * @note if FileOpenMode::Create provided @a access and other mode will
     * be ignore
     *
     * @throw Exception 当 @a openMode 是 FileOpenMode::Create 但是 @a fileMode
     * 是 nullopt
     * @sa
     * - PosixFileAccessMode
     * - SimplifiedFileOpenMode
     * - FileModes
     */
    FileOpenConfig(
        PosixFileAccessMode const& access,
        SimplifiedFileOpenFlag const& openFlag,
        PosixFileModes const* fileMode = nullptr);
    /// @todo ctor with extra open flags
    FileOpenConfig(
        PosixFileAccessMode const& access,
        SimplifiedFileOpenFlag const& openFlag,
        PosixFileOpenFlags const& extraOpenFlags);
    /// 获取文件打开 flag
    inline int getFlags() const { return this->flags; }
    /// 获取文件创建权限
    inline boost::shared_ptr<PosixFileModes> getMode() const noexcept {
        return this->mode; }
private:
    int flags;///< 文件打开 flags
    boost::shared_ptr<PosixFileModes> mode{ nullptr };///< 文件创建权限
};
/**
 * @struct File
 * 文件 IO
 * @note Auto close when instance release
 */
struct File {
    File(std::string const& filename = "");
    /// Close file when dtor
    virtual ~File();
    File(File const& other) = delete;/* obj no copy */
    File& operator=(File const& other) = delete;
    /**
     * @note if current file is open and new @a filename != filename
     * current file will be close
     */
    void setFilename(std::string const& filename) noexcept;
    std::string getFilename() const;
    /**
     * @return
     * - 1 when is directory
     * - 0 when not directory
     * - -errno when error
     */
    int isDirectory() const noexcept;
    bool isOpen() const noexcept;
    /// get total file size
    ssize_t size() const noexcept;
    /// get current io position
    ssize_t ioPostion() const noexcept;
    /// get current rest io size
    ssize_t ioRestPostion() const noexcept;
    /// Set fd postion to begin, return >= 0 when succes else -errno
    int jump2Begin() noexcept;
    /**
     * Set fd postion to current + @a offset
     * @return >= when succes else -errno
     */
    int jump2Offset(long const offset) noexcept;
    /**
     * Set fd postion to end + @a offset
     * @note @a offset should <= 0
     * @return >= 0 when succes else -errno
     */
    int rjump2Offset(long const offset) noexcept;
    /**
     * Open this file use @a openConfig
     * @param openConfig file open flag and create mode(when need)
     * @return >= 0 when succes else -errno
     */
    int open(FileOpenConfig const& openConfig);
    /// Close this file
    void close() noexcept;
    /**
     * read once @a size byes
     * @param size if >= 0 read @a size byes, if < 0 and real file rest size
     * <= kFileIoUpperBound read all, else if size < 0 and real file rest size
     * > kFileIoUpperBound read fail please use another read!
     * @param checkavail true to check available else false
     * @returns { code, readdata }
     */
    std::tuple<int, boost::shared_ptr<std::vector<uint8_t> > > read(
        ssize_t const size = -1, bool checkavail = true) noexcept;
    /**
     * NOTE better use traverse, especially for big file, implements with
     * mmap: to traverse current file.
     *
     * Read till end or max limit or error
     *
     * @param didRead The function to consume the data from current file
     * - If return true not read anymore and
     * - if total read < maxSize and not end will got a warning errno code (>0)
     * @param eachRead0 [0, kMaxFileRdMem]
     * - if 0, each read kFileIoUpperBound. default 128 MB.
     * - if > max use max, but NOTE too large mem maybe fail!!
     * @param limit if < 0 no limit
     * @returns { code, read bytes }
     * code:
     * - 0 full success
     * - > 0 success with warning
     * - < 0 fail
     * didread: how many bytes has read successfully
     * @note return value "read bytes" changed when read successfully,
     * even @a didRead return true;
     */
    std::tuple<int32_t, uint64_t> traverse(
        std::function<bool(uint8_t const* const data, uint32_t const size)>
            didRead,
        uint64_t const eachRead0 = kBigPerReadBytes,
        int64_t const limit = -1) noexcept;
    std::tuple<int, size_t> read(
        std::function<bool(uint8_t const* const data, size_t const size)>
            didRead,
        size_t const eachRead = sizeof(size_t) * 16 * 1024 * 1024,
        ssize_t const limit = -1) noexcept;
    /**
     * Write data to current file
     * @param data any data
     * @param eachWrite0 in bytes
     * - if < 0: one write
     * - if is 0: kFileIoUpperBound
     * - if > 0: each write @a eachWrite0 bytes
     * @param limit max write if >= 0
     * @returns { code, write bytes }
     */
    std::tuple<int, size_t> write(
        Any const& data,
        ssize_t const eachWrite0 = 0,
        ssize_t const limit = -1) noexcept;
    /// Better write method
    std::tuple<int32_t, uint64_t> write(
        void const* const data,
        uint64_t const maxWrite,
        int32_t const eachWrite0 = 0) noexcept;
    /**
     * write with callback when each write
     * @sa write
     */
    std::tuple<int, size_t> write(
        std::function<bool(size_t const wrote, size_t const rest)> didWrote,
        Any const& data,
        ssize_t const eachWrite = 0,
        ssize_t const limit = -1) noexcept;
    std::tuple<int, size_t> write(
        std::function<bool(size_t const wrote, size_t const rest)> didWrote,
        void const* const data,
        size_t const size,
        ssize_t const eachWrite = 0,
        ssize_t const limit = -1) noexcept;
    static int open(std::string const& filename, int const flag);
    static int open(
        std::string const& filename,
        int const flag,
        mode_t const mode);
private:
    struct IoBusyGuard {
        IoBusyGuard(bool& busy): busy(busy) { this->busy = true; }
        ~IoBusyGuard() { this->busy = false; }
    private:
        bool& busy;
    };
    inline bool __isOpen() const { return (this->fd >= 0); }
    ssize_t __size() const;
    mutable boost::shared_mutex filenameRwlock;
    mutable boost::shared_mutex fdRwlock;
    mutable std::mutex ioMutex;
    bool ioBusy{ false };
    std::string filename;
    int fd{ -1 };
    FileOpenConfig openConfig;///< open flags and create mode
};
}//namespace log
}//namespace cti
