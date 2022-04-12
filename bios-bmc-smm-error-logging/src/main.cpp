#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

static constexpr std::size_t intervalSecond = 1;

void read(boost::asio::steady_timer* t)
{
    /* This will run every 1 second */
    t->expires_at(t->expiry() + boost::asio::chrono::seconds(intervalSecond));
    t->async_wait(boost::bind(read, t));
}

int main()
{
    constexpr size_t intervalSecond = 1;

    boost::asio::io_context io;
    boost::asio::steady_timer t(io,
                                boost::asio::chrono::seconds(intervalSecond));

    t.async_wait(boost::bind(read, &t));

    io.run();

    return 0;
}