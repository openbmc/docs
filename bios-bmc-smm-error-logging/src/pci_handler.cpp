#include "pci_handler.hpp"

#include "internal/sys.hpp"

#include <fcntl.h>
#include <sys/mman.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace bmcBiosBuffer
{

bool PciDataHandler::open()
{
    static constexpr auto devmem = "/dev/mem";

    mappedFd = sys->open(devmem, O_RDWR | O_SYNC);
    if (mappedFd == -1)
    {
        std::fprintf(stderr, "PciDataHandler::Unable to open /dev/mem");
        return false;
    }

    mapped = reinterpret_cast<uint8_t*>(
        sys->mmap(0, memoryRegionSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                  mappedFd, regionAddress));
    if (mapped == MAP_FAILED)
    {
        sys->close(mappedFd);
        mappedFd = -1;
        mapped = nullptr;

        std::fprintf(stderr, "PciDataHandler::Unable to map region");
        return false;
    }

    return true;
}

std::vector<std::uint8_t> PciDataHandler::read(const std::uint32_t offset,
                                               const std::uint32_t length)
{
    std::vector<std::uint8_t> results(length);
    std::uint16_t finalLength =
        (offset + length < std::numeric_limits<uint16_t>::max())
            ? length
            : (std::numeric_limits<uint16_t>::max() - length);
    std::memcpy(results.data(), mapped + offset, finalLength);

    return results;
}

void PciDataHandler::write(const std::uint32_t offset,
                           const std::span<const uint8_t> bytes)
{
    const size_t bytesSize = bytes.size();
    std::uint16_t finalLength =
        (offset + bytesSize < std::numeric_limits<uint16_t>::max())
            ? bytesSize
            : (std::numeric_limits<uint16_t>::max() - bytesSize);
    std::memcpy(mapped + offset, bytes.data(), finalLength);
}

bool PciDataHandler::close()
{
    if (mapped)
    {
        sys->munmap(mapped, memoryRegionSize);
        mapped = nullptr;
    }

    if (mappedFd != -1)
    {
        sys->close(mappedFd);
        mappedFd = -1;
    }

    return true;
}

} // namespace bmcBiosBuffer
