# OCP LED Policy Support

Author: Alexander Hansen
[alexander.hansen@9elements.com](mailto:alexander.hansen@9elements.com)

Other contributors: None

Created: July 22, 2024

## Problem Description

OpenBMC must support the [OCP LED Policy](#background-and-references). Currently
there are some problematic cases in which the Policy is not supported by
existing services like phosphor-led-manager.

### Example ([Spec](#background-and-references) 6.2. System General Status)

Defined states:

| STS (blue) | FAULT (amber) | NAME                                            |
| ---------- | ------------- | ----------------------------------------------- |
| 1          | 0             | "All OK"                                        |
| 0          | 1             | "Module missing / fault"                        |
| 1          | 1             | LED Priority (cannot represent the fault state) |

Example sequence | STS | FAULT | action | | - | - | --- | | 0 | 0 | initial
state | | 1 | 0 | assert ok | | 1 | 1 | assert fault | | 1 | 1 | final state |

When for example both of these groups are asserted, the change in LED state will
be determined by the LED priority. The result is not allowed in the state
diagram in [Spec](#background-and-references).

In this trivial example we can fix it by just assigning Priority "Off" to the
led-blue. But that's not possible at the time of writing (commit
[bdbfcde](https://github.com/openbmc/phosphor-led-manager/commit/bdbfcde10520fc841b44d1e777c353a698b774ff)).

There are other examples with more LEDs where that alone will not fix the issue.

### Example ([Spec](#background-and-references) 6.4 PSU Status)

Here we pretend that LED Priority "Off" is allowed and discover that it will not
be sufficient to fix the issue.

#### Defined states

| AC OK (blue) | FAULT (amber) | LOW V (amber) | BACK UP (amber) | State                     |
| ------------ | ------------- | ------------- | --------------- | ------------------------- |
| 1            | 0             | 0             | 0               | "AC OK"                   |
| 0            | 1             | 0             | 0               | "AC Fault"                |
| 0            | 1             | 1             | 0               | "AC Under Voltage"        |
| 0            | 0             | 0             | 1               | "Backup due to AC Outage" |

#### LED Priority

| AC OK (blue) | FAULT (amber) | LOW V (amber) | BACK UP (amber) | State                     |
| ------------ | ------------- | ------------- | --------------- | ------------------------- |
| 0            | 0             | 0             | 1               | "Backup due to AC Outage" |

#### Example Sequence

| AC OK (blue) | FAULT (amber) | LOW V (amber) | BACK UP (amber) | State                        |
| ------------ | ------------- | ------------- | --------------- | ---------------------------- |
| 0            | 0             | 0             | 1               | LED Priority (for reference) |
| 0            | 0             | 0             | 0               | initial state                |
| 1            | 0             | 0             | 0               | assert "AC OK"               |
| 0            | 0             | 0             | 0               | assert "AC Fault"            |
| 0            | 0             | 0             | 0               | final state                  |

The final state is undefined according to [Spec](#background-and-references)
6.4.

The different possible LED priorities do not matter here, since "Backup due to
AC Outage" could very well be the highest priority state. And choosing any other
LED Priority will prevent it from being applied completely when any other state
is asserted simultaneously.

By now it becomes clear that what's needed is a way to define priorities in
terms of groups and not single LEDs.

## Background and References

Spec
[OCP Panel Indicator Specification rev1.0 (pdf)](http://files.opencompute.org/oc/public.php?service=files&t=65c02b1c6d59188351357cfb232cbfaa&download)
Spec
[OCP Panel Indicator Specification rev1.0 (presentation)](https://www.opencompute.org/files/OCP18-EngWorkshop-Indicator-Specification-Proposal.pdf)

Quick summary of what's important for us here:

### Permitted Indicator Colors

- Blue
- Amber
- Combined Blue/Amber in tight spaces

### Permitted Indicator States

- OFF
- ON
- BLINK (500ms on, 500ms off)

### Permitted Indicator Count on Component

- 1 blue LED
- 1 to n amber LED to communicate multiple fault conditions OR
- 1 blue/amber LED

## Requirements

- Ability to prioritize the different LED Groups and thus possible indicator
  states.
- An Indicator should not be in an inconsistent state. It can display the states
  as per the [Spec](#background-and-references)
- Other services cannot be trusted to keep the assertion of the LED groups in a
  consistent state. The indicator state must be consistent even when conflicting
  LED groups are asserted.

## Proposed Design

Extend the concept of "Priority" to groups in phosphor-led-manager. This allows
users to have configurations like:

1. OK
2. Fault 1
3. Fault 2
4. Locate

with locating and fault states having higher priorities. The LEDs would always
be in a consistent state. The change can be done in a backwards-compatible way.

The group priority and led priority can be mutually exclusive in configuration
to prevent any issues arising from mixed use. So the existing configs continue
to work with individual LED Priority and when a group priority is assigned, we
assume the user wants to use that for configuration.

## Alternatives Considered

- Extend phosphor-led-sysfs to expose 2 LEDs (blue/amber) as a single LED.

  - This does not work since there may be n different fault LEDs as per
    [Spec](#background-and-references)
  - Also this is basically lying to ourselves and making it difficult for other
    sw to get any meaningful info about LEDs from dbus anymore.

- Allow Priority "Off" for LED config.

  - This only solves the issue for very simple configurations.
  - Individual LED priorities are hard to think about when multiple LEDs form an
    indicator

- Allow the mixed use of group and individual led priority

  - This will require considering more edge cases arising from the mixed use.
  - Not aware of a use-case which would benefit from mixed use.

- Allow each LED to configure the priority of groups it represents, instead of
  just one state.

  - e.g. "Priority": ["enclosure_identify", "fault", "power"]
  - This config would have to be repeated on each instance of an LED
  - Or assumed that the first instance defines it?
  - Would need to check for equality of all these priority lists for an LED
  - This does not solve the problem of a group being represented incompletely
    - For example it is possible for 2 LEDs belonging to the same group to
      prioritize these groups differently in their priority list.

- Allow configuring an "indicator" that's comprised of multiple LEDs and then
  just define states for that indicator.

  - Need to translate these states to groups anyways to be compatible with the
    existing internal data structures
  - Handle the case of member LEDs of that indicator also being members of other
    groups
  - This is basically the same as group priorities but with additional overhead
    in config parsing

- Only display the group that was last asserted, in case of conflicting groups
  - This is undesirable since there are groups which are more important to
    display

## Impacts

Need to

- extend docs on how to configure LED group priority
- implement the LED group priority
- write unit tests for LED group priority
- perhaps change some configs to use this new feature
  - this is optional as the change is backwards-compatible

There should be no impact on d-bus except if we choose to expose the led group
priority.

### Organizational

- Does this repository require a new repository?
  - No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  - phosphor-led-manager
  - optionally openbmc to upgrade existing configs
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

How will this be tested? How will this feature impact CI testing?

The feature can easily be unit-tested.
