# uart-mux-support design

Author: Alexander Hansen <alexander.hansen@9elements.com>

Other contributors: Andrew Jeffery <andrew@codeconstruct.com.au> @arj, Jeremy
Kerr <jk@ozlabs.org>

Created: June 17, 2024

## Problem Description

Some hardware configurations feature a UART mux which can be switched via GPIOs.
To support this configuration, obmc-console needs to provide a method for
console selection to avoid manually setting GPIOs.

## Background and References

There are already [open changes for obmc-console][obmc-console-uart-mux-series]
but it has been determined that this feature needs a design document.

[obmc-console-uart-mux-series]:
  https://gerrit.openbmc.org/c/openbmc/obmc-console/+/71864

The background here is that there are some design choices which may affect other
subprojects - not in the way of causing regression, but later when the mentioned
hardware configuration needs to be supported in those projects.

## Requirements

- The user can select a console to be muxed

- Platform policy (whichever service implements it) can select the appropriate
  console depending on the host state and other information.

- It is clear to whoever is reading the logs of that console when a console was
  connected or disconnected via mux control. There should be no inexplicable
  gaps in log files.

- The mux configuration can be specified in a single file

- Console selection (implies mux control) must be possible from an external
  application.

The scope of this change is obmc-console and other projects which rely on the
APIs exposed by it.

The change will not affect users who do not have this hardware configuration.

## Design Considerations

There are a number of choices available for adding mux support into
obmc-console:

1. What the "connection endpoint" (Unix domain socket, D-Bus object) represents.
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

   1. Active connections prevent the mux state from being changed
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

### Scenarios and Use Cases

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

### Discussion

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

## Implementation Considerations

### How are muxed consoles represented on D-Bus?

Every console will have it's own D-Bus name, as this is backwards-compatible
with the current implementation.

Multiple consoles can be represented as a split- or unified- object tree.

### Tradeoffs of unified vs split object tree on D-Bus

In split-tree, it is not clear which consoles all belong to one UART mux, but in
unified-tree, this is clear.

In unified-tree, one console is reachable via the D-Bus name of another,
effectively creating multiple ways of doing something.

Example:

```
busctl set-property xyz.openbmc_project.Console.host1 \
/xyz/openbmc_project/console/host2 \
xyz.openbmc_project.Console.Access Connect ""
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

The choice of representation impacts how the mux can be described on D-Bus,
which is necessary if the out-of-band command strategy (2.1) is chosen. Two
possibilities for exposing an out-of-band mux control on D-Bus are:

1. Implement an interface on each console object that defines a boolean `Active`
   property, and an `Activate()` method. The `Activate()` method, by nature of
   being implemented on the console object, has all the context it needs to
   switch the mux without requiring caller-supplied parameters. The `Activate`
   property is `true` when the mux is configured for the console of interest,
   and `false` otherwise. A `PropertiesChanged` D-Bus signal for the `Active`
   variable may alert local users to changes of mux state.

2. Implement a `Mux` interface on an object common to all consoles exposed by
   the mux. The `Mux` interface might have a writable string `Selected` property
   that represents the state of the mux and provides a mechanism to switch it to
   a given console.

These have both been [discussed on an existing patch to
phosphor-dbus-interfaces][pdi-uart-mux-control-interface].

[pdi-uart-mux-control-interface]:
  https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/71878/comment/dd34b099_66dbc49e/

The second approach is quite explicit - directly representing the mux state
makes it easy to discover the state of the system. However, it motivates the
choice of a unified object tree to provide a common object path to host the
`Mux` interface (e.g. at `/xyz/openbmc_project/console`). This is desired to
avoid an alternative instance of the "multiple representations of one thing"
problem highlighted in the discussion of claiming multiple bus names for the
unified object tree: If the tree isn't unified, this `Mux` interface would have
to be represented and synchronised on objects across multiple D-Bus connections.

The first approach doesn't have this limitation. However, it does have the
trade-off previously mentioned, that it's unclear how any of the consoles in the
system are related, and what the impact might be of activating any one of them.

Choosing a strategy for D-Bus representation is required if we add to the D-Bus
API, i.e. with the out-of-band command design point (2.1). However, the choice
becomes more of an implementation detail if either of design options 2.2 or 2.3
are selected. The choice in those cases is instead motivated by the level of
clarity we desire in describing the relationships between consoles.

## Pruning the Design Decision Tree

To help shape the choices here, we have the existing behaviours of obmc-console
[discussed on the PDI patch][pdi-uart-mux-control-interface]:

1. We already have support for concurrent console server instances

2. Concurrent console support is implemented as one obmc-console-server process
   per Linux TTY device

3. As each Linux TTY device is paired with its obmc-console-server process, each
   obmc-console-server DBus connection needs a unique name

4. We use the unique console-ids to name global resources, including both the
   DBus connection and the instance's unix domain socket.

As in the linked discussion, given the `console-id` value really represents
what's at the remote end of the BMC's TTY device for regular unmuxed consoles,
it stands to reason that we should continue this strategy for muxed consoles.
Taking this approach avoids adding a new endpoint ABI to obmc-console and
eliminates design options A-D inclusive.

Further, on the basis of frustrating behaviour in the face of lingering network
connections, preventing mux changes on the grounds of an existing connection
seems like a bad path forward.

This leaves us with design options `F`, `G`, `I`, and `J`, which are
differentiated by how the mux is switched, and its effect on already-connected
clients.

Concentrating on how the mux is switched, based on the discussion about the
D-Bus representation above, the discussion on the PDI patch, and the impact on
related applications, it's reasonable to say there are some complications with
the out-of-band command method (2.1).

By contrast we can consider the alternative: We make the mux state reflect the
endpoint of the most recent connection. This has the benefit of functioning for
both the Unix domain socket and DBus access with no further effort. Neither
bmcweb nor phosphor-net-ipmid need be patched. The choice also eliminates the
D-Bus complications mentioned above as there's no need for the additional DBus
interface.

This reasoning leaves us the choice of design options `F` and `G`.

`F` and `G` are differentiated by whether or not we drop connections on
endpoints that are not the endpoint selected by the mux. There's been some back
and forth on that subject elsewhere[[1][drop-connections-discussion-1]]
[[2][drop-connections-discussion-2]], but it seems that not disconnecting
clients is effectively a worse implementation of design option `C`, which we've
already eliminated. It's worse than `C` because instead of 1 connection we could
have `N` connections for `N` mux ports, `(N - 1)` of which are idle. Not only
that, but the `(N - 1)` connections are effectively zombies, as they have no way
to switch the mux back to their associated port without establishing yet another
connection. It follows that if we're establishing a subsequent connection in
order to switch the mux we may as well disconnect the existing session, in which
case it may as well have been disconnected when the mux switched away to begin
with[^1].

[drop-connections-discussion-1]:
  https://gerrit.openbmc.org/c/openbmc/obmc-console/+/71228/comment/62a5fce9_60c3ad3e/
[drop-connections-discussion-2]:
  https://gerrit.openbmc.org/c/openbmc/obmc-console/+/71867/comment/756f0abe_5ebe8d66/

[^1]: which also saves resources

These arguments combined eliminate all but option `F`. It seems to sit at a neat
nexus in terms of both existing ABI, desired behaviour, and implementation
complexity.

Addendum: Discussions so far have been are around a _minimal_ design that
achieves the desired console behaviour. It's worth noting that design option `F`
(connection-based mux control which disconnects conflicting clients) allows us
to _optionally_ implement an out-of-band command interface in addition, because
the observable behaviour is no different to a new connection being accepted:
conflicting clients are disconnected and the mux is switched. This may be
helpful to implement platform policy around logging.

## Proposed Design

It's proposed that we use one obmc-console-server process to expose the `N`
consoles connected to a UART mux, where each console represents one mux port.
The mux is switched based on the endpoint of the most recent client connection,
and any conflicting clients are disconnected. This is design option `F` in the
table above.

The internal datastructures of obmc-console will change to accomodate the
design.

We will use one config file for the `N` muxed consoles. The configuration will
provide a similar approach for specifying the mux GPIOs to that used by [the
i2c-mux-gpio devicetree binding][linux-i2c-mux-gpio].

[linux-i2c-mux-gpio]:
  https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings/i2c/i2c-mux-gpio.yaml?h=v6.9#n12

Below is a block diagram of the relationships between the software and hardware
components:

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

To inform people who may be reading log files for a console, connection and
disconnection events of a console via mux control will produce messages for
clients and in log files.

Requirements are:

- Making it clear this message is from obmc-console
- Timestamp
- Indication of connected/disconnected

These messages are not meant as an API or reliable means to get information
about mux state. Any application on the other side of the uart could also
produce the exact same messages, even if unlikely.

The initial format of these messages will be something like:

```
[obmc-console] %Y-%m-%d %H:%M:%S UTC CONNECTED
[obmc-console] %Y-%m-%d %H:%M:%S UTC DISCONNECTED
```

for the connect and disconnect case.

For the D-Bus representation we choose the split tree, since that exposes the
least amount of information. If information about mux topology is needed by some
other project, we can always expose that later via additional properties.

## Other Alternatives Considered

### Kernel implementation

Did not do that since the support can be implemented in userspace. Also it may
not be merged since the hardware configuration it supports may not be widely
available. It may be better to have a userspace implementation to refer back to
in case someone wants to do a kernel implementation later.

### Multiple obmc-console-server processes for the multiple consoles

This was considered and implemented is a PoC, but discarded later as it would be
easier to synchronize everything in a single process.

### Multiple configuration files for multiple consoles

This was considered but it would duplicate configuration, like the definition of
the mux GPIOs. Inconsistencies across the files would also need to be managed.

## Impacts

### API Impact

### Performance Impact

Minimal to none.

### Developer Impact

Minimal. Existing users do not need to change anything about their
configuration.

### Organizational

- Does this repository require a new repository? No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  obmc-console, docs
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

There are already integration tests for this feature available on gerrit.
