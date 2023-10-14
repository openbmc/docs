# HTTP Library

Author: Abhilash Raju

Other contributors: None (Expecting other contributors)

Created: Nov 16, 2024

## Problem Description

OpenBMC currently lacks a reusable HTTPS library that can be leveraged 
across various services. This limitation not only hinders the ability 
to establish secure, encrypted communication channels but also 
complicates development efforts. By developing a common HTTPS library, 
we can streamline secure communication processes, enhance overall 
security, and simplify development and maintenance, making it easier 
for developers to implement and manage secure connections.

Potential example use cases for the need of a reusable HTTPS library 
include:

- **Auth Server**: Delegate the authentication and authorization 
     responsibilities from Bmcweb to a dedicated auth server. This requires 
     establishing a secure, encrypted communication channel between Bmcweb 
     and Phosphor User Manager (PUM).

- **Redfish Client**: Enable services within the BMC to make Redfish/REST 
     requests to services on other BMCs, facilitating aggregation and 
     inter-BMC communication.

- **Redfish Event Handling**: Offload the responsibility of delivering 
     Redfish events from Bmcweb to a dedicated service specifically designed 
     for managing these events.

- **Offloading Logs and Telemetry Data**: Implement a mechanism to 
     periodically transfer logs and telemetry data to external telemetry 
     servers for better data management and analysis.

## Background and References

Bmcweb is an HTTPS server application used within OpenBMC to handle 
web-based management interfaces. However, it is designed as a standalone 
application rather than a reusable library, limiting its flexibility and 
reusability across different services. Additionally, Bmcweb lacks 
coroutine support, which can complicate asynchronous programming and 
reduce efficiency in handling concurrent operations.

Boost::beast is another library used for HTTP and WebSocket functionality, 
providing low-level abstractions for these protocols. While it is 
powerful, its low-level nature means it does not offer higher-level APIs 
that simplify the development of client and server applications. This can 
make it challenging for application developers to implement secure 
communication features efficiently.

By addressing these limitations, we can create a more versatile and 
maintainable OpenBMC services. This design aims to develop a library 
that works out of the box, enabling OpenBMC applications to establish 
secure communication both internally and with external servers.

## Requirements

The proposed HTTPS library must adhere to the following constraints and 
requirements:

1. **No External Dependencies**: The library should not introduce any new 
      external dependencies, ensuring it remains lightweight and easy to 
      integrate within the existing OpenBMC ecosystem.

2. **Useful Abstraction**: It should provide a useful abstraction over 
      existing OpenBMC libraries, leveraging their capabilities while 
      simplifying the development process for secure communication.

3. **Out-of-the-Box Coroutine Support**: Provide out-of-the-box support 
      for coroutines to facilitate the easy development of asynchronous 
      client and server applications. Server URL handlers will be coroutines 
      by default, and the client APIs will be co_awaitables by default. The 
      design aims to simplify the development of single-threaded services.

4. **Integration with sdbusplus Coroutine APIs**: Ensure seamless 
      integration with sdbusplus coroutine APIs, facilitating smooth 
      communication and interaction with other OpenBMC services.

5. **Secure Communication over Unix Domain Sockets**: Support secure 
      communication over Unix domain sockets, providing an additional layer 
      of security for inter-process communication within the BMC.

6. **Task Abstraction and Declarative Composition**: While not 
      mandatory, the evolution of the APIs should focus on enabling 
      task abstraction and its composition. This will make it easier 
      for developers to define and manage asynchronous operations in 
      a declarative manner.
      
7. **Low Learning Curve**: The API should have a low learning curve and 
aim to be similar to the coroutine APIs provided by other OpenBMC 
libraries, such as sdbusplus. This will ensure consistency across the 
OpenBMC ecosystem and make it easier for developers to adopt and use 
the new library without needing to learn a completely new programming 
model.

By meeting these requirements, the library will provide a robust and 
developer-friendly solution for secure communication within OpenBMC.

## Proposed Design

The proposed design leverages Boost/Asio and Boost/Beast to create a 
lightweight and modular HTTPS library. The library will use C++20 
coroutines to provide a modern and efficient asynchronous programming 
model, making it easy to integrate with other libraries and services 
within OpenBMC. This is a header-only library.

### Key Components

1. **Transport API**: The transport layer will support both Unix domain 
      sockets and TCP/IP, allowing developers to choose the appropriate 
      transport mechanism based on their needs. The transport APIs will be 
      designed to work seamlessly with the security and task models, 
      enabling easy switching between internal and external communication 
      with minimal application changes.

2. **Security API**: The security layer will provide APIs for 
      establishing secure communication channels using TLS. This API will 
      be modular, allowing developers to combine it with the transport API 
      as needed. By default, the library does not create any SSL context 
      internally. Application developers can create their own context as per 
      their needs and hand it over to the library to use. The security APIs 
      will leverage Boost.Asio's SSL support to ensure robust and secure 
      connections.

3. **Task Model**: The choice of coroutine-based APIs makes it easy for 
      application developers to make asynchronous calls as simple task 
      functions. This will provide useful abstractions to help developers 
      write code that appears synchronous, while the underlying 
      implementation manages asynchronous execution. Although not required 
      for the initial release, the API should eventually support the 
      composition of parallel and sequential task structures. This will 
      simplify development by reducing the complexity of nested callback 
      chains.

4. **Type Safety**: To enhance type safety and simplify the integration 
      of the HTTPS library, applications are allowed to use type-safe 
      classes for the implementation. Instances of these classes can be 
      passed directly to HTTP request/response objects with a single line 
      macro in the class definition. This approach ensures that data types 
      are checked at compile time, reducing runtime errors and improving 
      code maintainability.

By using this macro, developers can seamlessly integrate their custom 
data types with the HTTPS library, ensuring type safety and reducing 
boilerplate code.

By focusing on a modular design and providing declarative-style APIs, 
the proposed library will offer a flexible and developer-friendly 
solution for secure communication within OpenBMC.

A reference implementation for the above design choices can be found 
[here](https://github.com/abhilashraju/coroserver?tab=readme-ov-file#example-web-client-to-download-content-from-googlecom).

## Alternatives Considered

One alternative design considered was using a senders and receivers 
library. However, the APIs of such libraries are fully functional in 
style, which demands a steep learning curve for developers. Additionally, 
there are no proven networking libraries available that work well with 
senders and receivers, meaning we would need to mix Boost/Asio with 
senders and receivers. This approach introduces a new library dependency 
and a steep learning curve, making it less favorable. The added complexity 
of integrating these libraries led us to lean away from this design.

## Impacts

### API Impact

This is a standalone library, so it does not impact existing OpenBMC 
applications directly. Applications that require HTTPS capabilities can 
integrate this library to gain secure communication features.

### Security Impact

The library enhances security by providing a reusable HTTPS solution, 
ensuring encrypted communication channels for applications that adopt it.

### Documentation Impact

Comprehensive documentation will be required to guide developers on how 
to integrate and use the library effectively. This includes API 
references, usage examples, and best practices.

### Performance Impact

The use of single-threaded asynchronous programming with coroutines is 
expected to improve performance by reducing the overhead associated with 
multi-threading and deep nested callback chains.

### Developer Impact

Developers will benefit from a simplified and consistent approach to 
implementing secure communication. The coroutine-based APIs will make 
asynchronous programming more intuitive and manageable.

### Upgradability Impact

The modular design of the library ensures that future enhancements and 
updates can be integrated with minimal disruption to existing 
applications. The library's evolution will focus on maintaining backward 
compatibility while introducing new features.

### Organizational

A new repository will be created within the OpenBMC project to host the 
HTTPS library. This repository will serve as the central location for the 
library's source code, documentation, and issue tracking. Applications 
that require secure communication features can declare a dependency on 
this library, ensuring a consistent and reusable approach to HTTPS within 
the OpenBMC ecosystem.

By centralizing the library in its own repository, we can streamline 
development, maintenance, and collaboration efforts, making it easier for 
contributors to enhance and update the library over time.

## Testing

This library will be tested using unit test cases. An example folder will 
be maintained in the library repository both for documentation and testing 
purposes. Developers can use it as a starting point and compile it for 
their development environment to ensure that the library is compatible and 
dependencies are satisfied.
