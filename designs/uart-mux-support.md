# uart-mux-support design

Author: Alexander Hansen <alexander.hansen@9elements.com>

Other contributors: None

Created: June 17, 2024

## Problem Description

Some hardware configurations feature a uart mux which can be switched via gpios.
To support this configuration, there needs to be an api for console selection,
to avoid manually setting gpio values.

## Background and References

There are already open changes for obmc-console, but it has been determined that
this change needs a design document.

The background here is that there are some design choices which may affect other
subprojects.

Not in the way of causing regression, but later when the mentioned hardware
configuration needs to be supported in those projects.

## Requirements

- The user can manually select a console

- Platform Policy (whichever service implements it) can select the appropriate
  console depending on the host state and other information. This will influence
  logging, since only one console behind a mux can be logged at a time.

- It is clear to whoever is reading the logs of that console when a console was
  connected or disconnected via mux control. There should be no inexplicable
  gaps in log files.

- The mux configuration can be specified in a single file, the configuration
  shall be similar to how it is done with i2c-mux-gpio in the linux kernel.

- Console Selection (implies mux control action) should happen via dbus.

The scope of this change is obmc-console and other projects which rely on the
APIs exposed by it.

The change will not affect users who do not have this hardware configuration.

## Proposed Design

The idea is for obmc-console-server to expose the n consoles connected to a uart
mux. So there will be one process for multiple consoles.

The internal datastructures of obmc-console will change to accomodate that.
There may be one config file for n consoles.

```
                                          +--------------------+
                                          | server.conf        |
                                          +--------------------+
                                               |
                                               |
                                               |
                                               |
                                          +----+----+                                 +-----+     +-------+
                                          |         |                                 |     |     |       |
                                          |         |     +-------+     +-------+     |     +-----+ UART1 |
+-----------------------------------+     |         |     |       |     |       |     |     |     |       |
| xyz.openbmc_project.Console.host1 +-----+         +-----+ ttyS0 +-----+ UART0 +-----+     |     +-------+
+-----------------------------------+     |         |     |       |     |       |     |     |
                                          |  obmc   |     +-------+     +-------+     |     |
                                          | console |                                 | MUX |
                                          | server  |                   +-------+     |     |
+-----------------------------------+     |         |                   |       |     |     |
| xyz.openbmc_project.Console.host2 +-----+         +-------------------+ GPIO  +-----+     |     +-------+
+-----------------------------------+     |         |                   |       |     |     |     |       |
                                          |         |                   +-------+     |     +-----+ UART2 |
                                          |         |                                 |     |     |       |
                                          +----+----+                                 +-----+     +-------+

```

### Handling of Mux Control

To inform people who may be reading log files for a consoles, connection and
disconnection events of a console via mux control will produce messages for
clients and in log files.

### How is this represented on dbus?

Every console will have it's own dbus name, this is backwards-compatible with
the current implementation.

Multiple consoles can be represented as split-tree or unified-tree.

### Tradeoffs of unified vs split tree

In split-tree, it is not clear which consoles all belong to one uart mux, but in
unified-tree, this is clear.

In unified-tree, one console is reachable via the dbus name of another,
effectively creating multiple ways of doing something.

Example:

```
busctl set-property xyz.openbmc_project.Console.host1 \
/xyz/openbmc_project/console \
xyz.openbmc_project.Console.Mux Selected s "host2"
```

So a choice has to be made how to represent multiple consoles on dbus, and what
information needs to be exposed to other subprojects.

Unified Tree:

```
busctl tree --user xyz.openbmc_project.Console.host1
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/console
      ├─/xyz/openbmc_project/console/host1
      └─/xyz/openbmc_project/console/host2
```

Split Tree:

```
busctl tree --user xyz.openbmc_project.Console.host1
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/console
      └─/xyz/openbmc_project/console/host1

busctl tree --user xyz.openbmc_project.Console.host2
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/console
      └─/xyz/openbmc_project/console/host2
```

## Alternatives Considered

- Kernel implementation. Did not do that since the support can be implemented in
  userspace. Also it may not be merged since the hardware configuration it
  supports may not be widely available. It may be better to have a userspace
  implementation to refer back to in case someone wants to do a kernel
  implementation later.

- Multiple obmc-console-server processes for the multiple consoles. This was
  considered and implemented is a PoC, but discarded later as it would be easier
  to synchronize everything in a single process.

- Multiple configuration files for multiple consoles. This was considered but it
  would duplicate configuration, like the name of the mux gpios. Also then
  inconsistencies across the files would have to be handled.

## Impacts

### API Impact

New dbus interfaces, properties and methods may be created. There should be no
regression.

### Performance Impact

Minimal to none.

### Developer Impact

Minimal. Existing users do not need to change anything about their
configuration.

### Organizational

- Does this repository require a new repository? No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  obmc-console, phosphor-dbus-interfaces, docs
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

There are already integration tests for this feature available on gerrit.
