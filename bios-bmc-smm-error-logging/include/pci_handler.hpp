#pragma once

#include "internal/sys.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace bmcBiosBuffer
{

/**
 * Each data transport mechanism must implement the DataInterface.
 */
class DataInterface
{
  public:
    virtual ~DataInterface() = default;

    /**
     * Initialize data transport mechanism.  Calling this should be idempotent
     * if possible.
     *
     * @return true if successful
     */
    virtual bool open() = 0;

    /**
     * Read bytes from shared buffer (blocking call).
     *
     * @param[in] offset - offset to read from
     * @param[in] length - number of bytes to read
     * @return the bytes read
     */
    virtual std::vector<std::uint8_t> read(const std::uint32_t offset,
                                           const std::uint32_t length) = 0;

    /**
     * Write bytes to shread buffer.
     *
     * @param[in] offset - offset to write to
     * @param[in] bytes - byte vector of data.
     * @return bool - returns true on success.
     */
    virtual void write(const std::uint32_t offset,
                       const std::span<const uint8_t> bytes) = 0;

    /**
     * Close the data transport mechanism.
     *
     * @return true if successful
     */
    virtual bool close() = 0;
};

/**
 * Data handler for reading and writing data via the PCI bridge.
 *
 */
class PciDataHandler : public DataInterface
{
  public:
    PciDataHandler(std::uint32_t regionAddress, std::size_t regionSize,
                   const internal::Sys* sys = &internal::sys_impl) :
        regionAddress(regionAddress),
        memoryRegionSize(regionSize), sys(sys){};

    bool open() override;
    std::vector<std::uint8_t> read(std::uint32_t offset,
                                   std::uint32_t length) override;
    void write(const std::uint32_t offset,
               const std::span<const uint8_t> bytes) override;
    bool close();

  private:
    std::uint32_t regionAddress;
    std::uint32_t memoryRegionSize;
    const internal::Sys* sys;

    int mappedFd = -1;
    std::uint8_t* mapped = nullptr;
};

} // namespace bmcBiosBuffer
