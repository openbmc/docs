# Entity Manager Supported Configuration Schema

Author:
  Adriana Kobylak (anoo)

Primary assignee:
  Adriana Kobylak (anoo)

Other contributors:
  Ed Tanous (edtanous)

Created:
  May 3rd, 2021

## Problem Description
There is a desire to model the set of components that are supported in a
system. For example, a system may require particular set of DIMMs, specific CPU
models, or a minimum number of power supplies. By modeling the supported
configuration sets, monitoring applications can notify the user in case of
mismatches and optionally take actions such as powering off the system in the
case of insufficient power supplies to avoid damaging the system.

## Background and References


## Requirements
- The configurations sets applicable to each system should dynamically be
  loaded at runtime.
- Configuration sets follow a predefined schema so that monitoring applications
  are able to parse any new or changed configuration set.
- Configuration sets are represented by data files such as JSON, and their
  format is intuitive so that they can easily be updated by different people
  such as system owners or hardware engineers.

## Proposed Design
The schema would comprise of the following elements:
- Supported configuration set: An array of strings that lists the supported
  components for this configuration. Example, a chassis that supports two
  power supplies model "PowerSupply-1" would look like:
  "['My Chassis', 'PowerSupply-1', 'PowerSupply-1']"
- Severity: A list of enums that indicate if any action should be taken if the
  supported configuration set is not met. Example values: "Info", "Error",
  "Critical".
- Allow extra components: Flag that specifies if additional components would
  cause the specified supported configuration to fail.

### Open design questions (to be updated once they are resolved / agreed upon):
The schema would be implemented in the [Entity Manager repo][]. The supported
configuration schema would be an opt-in flag set at compile time that would
trigger the supported configuration JSON files to be included in the
compilation.

In order to dynamically appear as a D-Bus object, the Probe condition would
contain one or more of the components listed in the supported configuration
set. For the example above, a Probe condition could be a Chassis Inventory Item
with a Decorator Asset Model name of "My Chassis", OR a Power Supply Inventory
Item with a Decorator Asset Model name of "PowerSupply-1".

For requirements that are specific to one of the components, such as the power
supply input voltage, a separate JSON configuration file could be created, such
as PowerSupply-1-110V and PowerSupply-1-220V, and depending on which one the
system supports is the one listed in the supported configuration set.

This design does not currently model location such as slot position, where a
component X in slot 1 would be supported but if plugged in slot 2 would be
treated as unsupported.

## Alternatives Considered
Have the monitoring applications include hard-coded information about the
system and the components they support. For example, the power supply monitor
app would have have if/else statements for each system type it supports, such
as if system = "MyChasssis" then "Need to check for two Power Supplies model X".
This is not efficient since the application would need to be updated to include
changes, and the code would become bloated with system specific checks.

## Impacts
This design would impact the [Entity Manager repo][], and any monitoring app
that wants to enforce the supported configuration schema, such as the power
supply monitor in the [Phosphor Power repo][].

Adding an additional schema to the [Entity Manager repo][] usually results in a
small performance impact since there would be an additional JSON file to load,
this is true in general for any additional JSON configuration added.

## Testing
The generated D-Bus objects can be verified by inspection. Each monitoring app
can add additional tests when implementing enforcing the supported
configurations.

[Entity Manager repo]: https://github.com/openbmc/entity-manager/
[Phosphor Power repo]: https://github.com/openbmc/phosphor-power
