#include "pci_handler.hpp"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <memory>

static constexpr std::size_t intervalSecond = 1;
static constexpr size_t readSize = 75;

std::vector<std::uint8_t> lastBuffer(readSize, 0);

void initBuffer(
    const std::size_t bufferSize,
    const std::shared_ptr<bmcBiosBuffer::DataInterface>& dataInterface)
{
    dataInterface->open();
    std::vector<std::uint8_t> emptyBuffer(bufferSize, 0);
    dataInterface->write(0, emptyBuffer);
    dataInterface->close();
}

void read(boost::asio::steady_timer* t,
          const std::shared_ptr<bmcBiosBuffer::DataInterface>& dataInterface)
{
    dataInterface->open();

    std::vector<std::uint8_t> bytesRead = dataInterface->read(0, readSize);
    if (bytesRead != lastBuffer)
    {
        fprintf(stderr, "Read: ");
        for (std::uint8_t byte : bytesRead)
            fprintf(stderr, "0x%02x ", byte);
        fprintf(stderr, "\n");
        lastBuffer = bytesRead;
    }

    /* This will run every 1 second */
    t->expires_at(t->expiry() + boost::asio::chrono::seconds(intervalSecond));
    t->async_wait(boost::bind(read, t, dataInterface));
    dataInterface->close();
}

int main()
{
    constexpr std::size_t memoryRegionSize = 16 * 1024UL;
    constexpr std::size_t memoryRegionOffset = 0xF0848000;

    boost::asio::io_context io;
    boost::asio::steady_timer t(io,
                                boost::asio::chrono::seconds(intervalSecond));

    std::shared_ptr<bmcBiosBuffer::DataInterface> pciDataHandler =
        std::make_shared<bmcBiosBuffer::PciDataHandler>(memoryRegionOffset,
                                                        memoryRegionSize);

    initBuffer(memoryRegionSize, pciDataHandler);
    t.async_wait(boost::bind(read, &t, pciDataHandler));

    io.run();

    return 0;
}