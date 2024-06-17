# uart-mux-support design

Author: Alexander Hansen <alexander.hansen@9elements.com>

Other contributors: Andrew Jeffery <andrew@codeconstruct.com.au> @arj, Jeremy
Kerr <jk@ozlabs.org>

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

## Considerations

There are a number of choices available for adding mux support into
obmc-console:

1. What the "connection endpoint" (Unix domain socket, DBus object) represents.
   This could be either:

   1. The TTY device exposed by Linux
   2. The desired downstream mux port

2. How the mux state is controlled. We might control it by any of:

   1. An out-of-band command (e.g. via a DBus method that's somehow associated
      with the connection endpoint
   2. An in-band command (e.g. introducing an SSH-style escape-sequence)
   3. Selecting the mux port based on the endpoint to which the user has
      connected

3. The circumstances under which we allow the mux state to be changed

   1. Active connections prevents the mux state from being changed
   2. The mux state can always change but will terminate any existing
      conflicting connections
   3. The mux state can always change and has no impact on existing conflicting
      connections

4. Whether we want the data stream on a given connection to represent:
   1. The console IO regardless of the mux state
   2. The console IO isolated to a specific mux port

There are constraints on some combinations of these. For instance:

- If the connection endpoint represents the TTY device exposed by Linux (1.1)
  then we can't select the mux port based on the endpoint to which the user has
  connected (2.3) as we simply don't have the information required
- If the connection endpoint represents the desired downstream mux port (1.2)
  then it doesn't make sense to implement support for an in-band command to
  change the mux state (2.2) as it's a violation of the abstraction
- If the connection endpoint represents the desired downstream mux port (1.2)
  then it can't provide the console IO of another mux port (4.1) as that's
  contrary to the definition.

With these in mind we end up with the following table of design options:

| ID  | Connection Endpoint (1) | Mux Control Defined By (2) | Mux Control Policy (3)                           | Stream Data (4)   |
| --- | ----------------------- | -------------------------- | ------------------------------------------------ | ----------------- |
| A   | TTY (1.1)               | Out-of-band command (2.1)  | Active connections prevent mux change (3.1)      | Isolated (4.2)    |
| B   | TTY                     | Out-of-band command        | Mux change with disconnections (3.2)             | Isolated          |
| C   | TTY                     | Out-of-band command        | Mux change without disconnections (3.3)          | Multiplexed (4.1) |
| D   | TTY                     | In-band command (2.2)      | Mux change without disconnections                | Multiplexed       |
| E   | Mux port (1.2)          | Connection-based (2.3)     | Conflicting connections prevent mux change (3.1) | Isolated          |
| F   | Mux port                | Connection-based           | Mux change with disconnections                   | Isolated          |
| G   | Mux port                | Connection-based           | Mux change without disconnections                | Isolated          |
| H   | Mux port                | Out-of-band command        | Conflicting connections prevent mux change       | Isolated          |
| I   | Mux port                | Out-of-band command        | Mux change with disconnections                   | Isolated          |
| J   | Mux port                | Out-of-band command        | Mux change without disconnections                | Isolated          |

## Scenarios and Use Cases

1. A UART mux selecting between a satellite BMC on a blade and the blade host

   A software update is in progress on the satellite BMC and the mux has been
   switched to capture the output of whatever the satellite is printing. It is
   important to log the output of the update process to understand any failures
   that might result.

   While the satellite BMC update is in progress, a user chooses to connect to
   the host console.

2. A blade's satellite BMC, CPLD and host are all on separate ports of a UART
   mux, and relevant output from the blade's boot process must be captured

   The boot process for a blade requires a sequence of actions across its
   satellite BMC, CPLD and host. Each component contributes critical information
   about the boot process, which is output on the respective consoles at various
   points in time.

   For ease of correlation, their output should be logged together.

## Discussion

Scenario 1 is problematic. It highlights the fundamental concern of ownership of
the mux state. In the scenario the system is in a sensitive state where a
specific mux configuration is required (to output update progress from the
satellite BMC), but a user has shown intent for the selection of another (to
interact with the host console).

What should occur? And does this choice impact how we choose to control the mux?

Taking a connection-based approach to setting the mux state (2.3) will cause the
user connecting to the host console endpoint to immediately disrupt the update
progress output from the satellite BMC.

By contrast, by setting the mux state with an out-of-band command (2.1) and not
on the initiation of a connection (2.3), the user connecting to the host console
will not immediately disrupt the update progress output from the satellite BMC.

However, we can presume the user is connecting to the host console endpoint for
a reason. With extra actions, using the out-of-band command interface, they may
equally choose to switch the mux without regard for the system state, disrupting
the update progress output from the satellite BMC.

This highlights that the fundamental problem is access to the system by multiple
users who are neither coordinating with each other nor the system state. The
question that follows is:

Should it be the responsibility of obmc-console to coordinate otherwise
un-coordinated users?

This is a question of policy: How those users should be coordinated will likely
look very different based on concerns such as the role of the platform in a
larger system, the roles and needs of the users interacting with it, and the
concrete design of the platform itself.

obmc-console should implement a mechanism to control the mux state, but likely
shouldn't apply any policy governing access to the muxed consoles.

A further concern for the out-of-band command approach is its interactions with
other components exposing consoles:

1. The dropbear/obmc-console-client integration exposing consoles via SSH
2. [bmcweb](https://github.com/openbmc/bmcweb/blob/master/include/obmc_console.hpp)
3. [phosphor-net-ipmid](https://github.com/openbmc/phosphor-net-ipmid/blob/master/sol/sol_manager.hpp)

With the out-of-band command approach these components have to choose between:

- Not providing any capability to change the mux state; rather, they defer to
  making the user log in via SSH to affect the change themselves

- Expose some mechanism for setting the mux state in terms of their own external
  interfaces

- Assume that a user connecting to the exposed console endpoint wants select
  that console if it's behind a mux

The first assumes that SSH is exposed at all and accessible by users who need
access to the muxed consoles. It's not yet clear whether this is a reasonable
expectation.

The second assumes that these external interfaces have the capability to model
the problem. It's not yet clear that this is the case for either of IPMI or
Redfish, and it's not the case for serial over SSH.

The third implies that we must add capability to all three components to drive
the out-of-band command interface when they receive a connection for a given
console. The net result is no behavioural difference from obmc-console
implementing this itself (2.3), but increased complexity across the system.

# TODO: Rejig the sections below after review feedback on the above

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

- Console Selection (implies mux control) must be possible from an external
  application.

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
