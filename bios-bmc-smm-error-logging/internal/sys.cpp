#include "sys.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>

namespace internal
{

int SysImpl::open(const char* pathname, int flags) const
{
    return ::open(pathname, flags);
}

int SysImpl::read(int fd, void* buf, std::size_t count) const
{
    return static_cast<int>(::read(fd, buf, count));
}

int SysImpl::close(int fd) const
{
    return ::close(fd);
}

void* SysImpl::mmap(void* addr, std::size_t length, int prot, int flags, int fd,
                    off_t offset) const
{
    return ::mmap(addr, length, prot, flags, fd, offset);
}

int SysImpl::munmap(void* addr, std::size_t length) const
{
    return ::munmap(addr, length);
}

int SysImpl::ioctl(int fd, unsigned long request, void* param) const
{
    return ::ioctl(fd, request, param);
}

SysImpl sys_impl;

} // namespace internal

