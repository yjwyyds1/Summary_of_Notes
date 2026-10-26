// 目的：演示快照文件的“先写新版本 → 同步 → 切换名字”顺序。
// 前提：Linux/POSIX；演示目录在 /tmp；不声明任意存储设备上的绝对掉电保证。
// 观察：target 最终包含 new-snapshot；临时目录会被清理。
#include <cerrno>       // errno、EINTR
#include <cstdlib>      // mkdtemp
#include <fcntl.h>      // open、O_DIRECTORY、O_RDONLY、O_WRONLY
#include <iostream>     // cout
#include <stdexcept>    // runtime_error
#include <string>       // string
#include <unistd.h>     // write、fsync、close、read、rmdir
#include <cstdio>       // rename

int main() {
    char dir_template[] = "/tmp/bagu_replace_XXXXXX";
    char* dir = ::mkdtemp(dir_template); // 唯一演示目录，避免覆盖真实文件。
    if (!dir) throw std::runtime_error("mkdtemp failed");
    const std::string temp = std::string(dir) + "/snapshot.tmp";
    const std::string target = std::string(dir) + "/snapshot.dat";
    const int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd == -1) throw std::runtime_error("open temp failed");
    const std::string body = "new-snapshot";
    std::size_t done = 0;
    while (done < body.size()) {
        const ssize_t n = ::write(fd, body.data() + done, body.size() - done);
        if (n > 0) { done += static_cast<std::size_t>(n); continue; }
        if (n == -1 && errno == EINTR) continue;
        throw std::runtime_error("write failed");
    }
    if (::fsync(fd) == -1 || ::close(fd) == -1)
        throw std::runtime_error("file sync/close failed");
    if (::rename(temp.c_str(), target.c_str()) == -1)
        throw std::runtime_error("rename failed");
    const int dir_fd = ::open(dir, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1 || ::fsync(dir_fd) == -1 || ::close(dir_fd) == -1)
        throw std::runtime_error("directory sync failed");
    const int read_fd = ::open(target.c_str(), O_RDONLY);
    if (read_fd == -1) throw std::runtime_error("open target failed");
    char buffer[32]{};
    const ssize_t n = ::read(read_fd, buffer, sizeof(buffer));
    if (n < 0) throw std::runtime_error("read target failed");
    std::cout.write(buffer, n);
    std::cout << '\n';
    ::close(read_fd);
    ::unlink(target.c_str());
    ::rmdir(dir);
}
