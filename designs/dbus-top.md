
## Design Doc for dbus-top

Author: Adedeji Adebisi (ziniblessed@google.com)

Primary Assignee: Adedeji Adebisi 

Other contributors: Sui Chen (suichen@google.com) Alex Qiu (xqiu@google.com)

Created: May 17, 2021


## Problem Description

Design and Implementation of a top-like tool for DBus. This tool shows metrics
related to dbus connections such as message throughput, as well as system
information available through DBus such as the list of sensors, in an attempt to
understand some performance related issues on the BMC. The sensor information is
extracted from well-known DBus services such as the Object Mapper, Inventory
Manager, and Entity Manager. DBus-top shows the information conveniently that
might otherwise require the user to type long commands or scripts.

## Background and References

This tool adds to the set of command-line based tools that run directly on the
BMC. Using a tool that directly runs on the BMC, a user can debug problems
in-situ, without needing to extract data from the BMC to the host for offline
analysis.  This tool resembles a typical top-like user interface, giving the
user a top-down view of various DBus states. This tool is designed in the same
logic as similarly-purposed, existing tools, each focusing on one particular
field such as ctop (for containers), iotop (for I/O throughput), iftop (for
network traffic).

## Requirements

Constraints include: BMC consoles may not be capable of displaying all text
styles supported in ncurses Dependency on libncurses which might add ~130KB
overhead to the BMC’s Flash space.  Monitoring DBus should not incur so much
performance overhead as to distort measured values.  Users are OpenBMC users and
developers: This tool should help beginners learn the basics of OpenBMC, as well
as help seasoned developers quickly locate a problem.  Scope: This tool is
standalone, self-sufficient, with minimum dependency on existing software
packages (only depends on basic packages like systemd, libdbus, sdbus,
sdbusplus, and libncurses.) Data size, transaction rates, bandwidth: We will be
looking at typically hundreds of DBus messages per second and around 1 MB/s data
throughput. The BMC’s processing power and memory capacity are sufficient for
the scale of the data this tool is processing.


This tool should work on a live DBus as well as on a capture file (a pcap file).
This tool should work both on the host or the BMC. The UI design is scaled to
work with large numbers of sensors and different console sizes.

## Proposed Design

The proposed design is to layout a multi-window UI similar to how a tiled window
manager does: 

```
+----------------------------------------------------------------------------+
|Message Type          | msg/s        History                 (Total msg/s)  |
|Method Call             45.00        -  .       .       .       .    -2300  |
|Method Return           45.00        -  :       :       :       :    -1750  |
|Signal                  53.00        -  :       :       :       :    -1200  |
|Error                    0.00        -  :       :       :       :    -650   |
|Total                  142.99        -:::.....:::.....:::.....:::....-100   |
+-------------------------------------+--------------------------------------+
| Columns 1-2 of 6      84 sensors    |   Msg/s   Sender  Destination        |
|   vddq_efgh_out     vddcr_cpu1_in   |   7.50    :1.14   :1.48              |
|   vddcr_cpu0_out    vddcr_soc1_out  |   7.50    :1.14   org.freedesktop.DB>|
|   vddq_mnop_out     vddq_efgh_in    |   7.50    :1.48   :1.52              |
|   vddcr_soc0_out    vddq_abcd_in    |   7.50    :1.48   org.freedesktop.DB>|
|   vddq_ijkl_in      vddq_efgh_out   |   1.00    :1.48   xyz.openbmc_projec>|
|   vddcr_soc0_in     hotswap_pout    |   1.00    :1.48   xyz.openbmc_projec>|
|   vddcr_cpu0_in     vddcr_cpu1_in   |   1.00    :1.48   xyz.openbmc_projec>|
|   vddq_ijkl_out     vddq_efgh_in    |   5.00    :1.48   xyz.openbmc_projec>|
|   vddcr_soc1_in     vddq_ijkl_out   |   5.00    :1.48   xyz.openbmc_projec>|
|   hotswap_iout      vddcr_soc0_out  |   1.00    :1.48   xyz.openbmc_projec>|
|   vddq_abcd_in      vddcr_soc1_in   |   7.50    :1.48   xyz.openbmc_projec>|
|   vddq_mnop_in      vddq_mnop_in    |   7.50    :1.52   xyz.openbmc_projec>|
|   vddcr_cpu1_out    vddcr_cpu1_out  |   1.00    :1.70   (null)             |
|   vddq_abcd_out     vddcr_cpu0_out  |   1.00    :1.70   :1.48              |
+-------------------------------------+--------------------------------------+
 Fri Jul  2 15:50:08 2021                                    PRESS ? FOR HELP
```

The UI has 3 windows:

1. Top, summary window: This window is divided into two sections, the dbus
   traffic dump that shows the method calls, method returns, signals and errors
   in every dbus callback. This is significant because it shows the latency
   between every call back and the other section that shows the graph can
   benefit from this. The other part shows a history graph of these
   corresponding data and it is designed this way to show users a pictorial
   representation of the callback metrics. 

2. Bottom-left, sensor detail window: This window has two states:
  - The window shows the list of all sensors (as defined by DBus objects
    implementing the xyz.openbmc_project.Sensor.Value) by default
  - When a sensor is selected, it shows the corresponding details including
    sensor name, the dbus connection, PID and the dbus service associated with
    the sensor. This will be helpful in understanding what is connected to the
    sensor in a case of performance related to the sensor.

The following is how the window might look once a sensor has been selected: 

```
+-------------------------------------------------+
| Details for sensor cputemp0        Sensor 1/192 |
| [DBus Info]                                     |
| DBus Connection :  :1.234                       |
| DBus Service    :  xyz.openbmc_project...       |
| DBus Object Path:  /path/to/object              |
|                                                 |
| [Inventory Info]                                |
| Association:                                    |
|  - chassis: all_sensors                         |
|                                                 |
| [Visibility]                                    |
|  - IPMI SEL: Yes                                |
|  - Object Mapper: Yes                           |
|  - Sensor Daemon: Yes                           |
+-------------------------------------------------+
```

3. Bottom-right, DBus stats list view window: This window shows the stats of
DBus connections in a list, aggregated by the fields of the user’s choice. These
fields include sender, destination, path, interface, and message type.

A menu is available that allows the user to select what fields to use to
aggregate the entries in the window.  

```
+----------------------------------------+
|Available Fields          Active Fields |
|                                        |
|Interface                 Sender        |
|Path                 ---> Destination   |
|Message Type         <---               |
|Sender Process Name                     |
+----------------------------------------+
```

The windows can be resized, or shown/hidden individually.


Overview of the Program Structure: 

```
      UI Thread               Analysis Thread          Capture Thread
     _______^_______        _________^__________      _________^______
    /               \      /                    \    /                \

          +-+-------+     +----------+   +-------+      +-----------+
+-----+   | |User   |     | Call     | ,-|Message|<-.   |Become     |
|User |   | |Setting|-. S | ObjMapper| | |Handler|  |M  |DBus       |
|Input|   | `-------+ | o | Inv.Mgr  | | +-------+  |eD |Monitor    |
+-----+   |         | |Kr | EntityMgr| |     |      |sB +-----------+
   |      |Frontend | |et +----------+ |     V      |su       |
   `----->|   UI    | |y    |          | +-------+  |as       V
   Key    |         | |s    `----------->|Method |  |g  +------------+
   Stroke |         | `-->+---------+  | |Call   |  |e  |DBus Capture|
          |         |<-.  |Aggregate|  | |Tracker|  |s  +------------+
          +---------+  |  |         |<-' +-------+  |      |
           |     |     |  +---------+        |      +------'
           V     V     |       |             |      |
   +--------+ +-----+  |       V             V      |
   |ncurses-| |Text-|  |   +-------+     +-------+  |   +-----------+
   |based   | |based|  +---|Sensor |     |Sensor |  `---|PCAP Replay|
   |UI      | |UI   |  |   |Stats  |     |Details|      +-----------+
   +--------+ +-----+  |   +-------+     +-------+
                       |                     |
                       `---------------------'
```    
  
## Alternatives Considered

Non-Curses UI: This would help avoid dependence on libncurses, which some BMCs
might not have. The downside is that the rendering routines will need to be
reworked; one possible way is to overload ncurses functions and maintain a text
buffer by ourselves.

## Impacts

API impact: none Security impact: none Performance impact: Will only have a
small impact when the program is running. No impact when it’s not running.
Developer impact: for machines that do not install libncurses by default, adding
this tool might incur a ~130KB overhead due to the libncurses library. No space
overhead if the machine already includes libncurses.

## Testing

Individual functionalities will be tested with unit tests. Functional tests will
be done using PCAP files. CI testing is not affected since it doesn’t modify the
build process of any other package.
