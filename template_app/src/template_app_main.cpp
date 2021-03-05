#include <template_app.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

int main(int , char** )
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name("xyz.openbmc_project.TemplateApp");

    auto server = sdbusplus::asio::object_server(conn);

    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/xyz/openbmc_project/template_app",
                             "xyz.openbmc_project.template");

    iface->register_property("IntegerValue", 23, changeIntegerValue);


    iface->register_method("UnlockDoor", unlockDoor);
}