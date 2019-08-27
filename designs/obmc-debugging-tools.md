# OBMC Debugging tools

Author:
  Wilfred Smith <wilfredsmith@fb.com> <wilfredsmith>

Primary assignee:
 None

Other contributors:
  Vijay Khemka <vijaykhemka@fb.com> <vijay>
   Amithash Prasad <amithash@fb.com> <amithash>

Created:
  2019-08-16

## Problem Description
For debugging, it is useful to be able to quickly and easily perform
certain operations from commandline: retrieving FRU information, sending
commands to devices connected via IPMB, and retrieving sensor data,
among them. These low-level tools provide a simple interface for
retrieving this information using D-Bus that is similar to legacy
retrieval methods and does not require lengthy commands to do so.

## Background and References
None
## Requirements
The primary users of this utility are those debugging issues on the
server as well as bmc, for example, during bringup or when the
full network stack is not available.

## Proposed Design
- Sensor information will be retrieved through sensor interfaces exposed
  on the D-Bus and from a historical sensor information store
- IPMB commands will be issued through the
  xyz.openbmc_project.Ipmi.Channel.Ipmb’s sendRequest interface
- FRU information will be retrieved from the
  xyz.openbmc_project.FruDevice service

## Alternatives Considered
Considered using redfishtool.
- Reintroduces dependency on Python on BMC.
- Requires many more components to be fully functional before information
  can be retrieved

Considering leveraging Telemetry functionality for historical information
-Telemetry is still in planning phases and has substantially more complex
 goals than required for this tool. The historical collection daemon
 required by this tool may become a data source for Telemetry.

More information regarding the Platform Telemetry and Health Monitoring
Workgroup may be found here. [^1]

## Impacts
None

## Testing
Utilities will be published alongside unit tests, code coverage results
and sufficient documentation to allow quick adaptation to other D-Bus
hierarchies.

[^1]:	[https://github.com/openbmc/openbmc/wiki/Platform-telemetry-and
                 -health-monitoring-Work-Group][1]
