# OCP LED Policy Support

Author: Alexander Hansen
[alexander.hansen@9elements.com](mailto:alexander.hansen@9elements.com)

Other contributors: None

Created: July 22, 2024

## Problem Description

OpenBMC must support the OCP LED Policy. Currently there are some problematic
cases in which the Policy is not supported by existing services like
phosphor-led-manager.

### Example ([1]: 6.2. System General Status)

```
LEDs: "STS", "FAULT"
colors: blue, amber
Defined states:
10 "All OK"
01 "Module missing / fault"

11 (LED priorities cannot represent the fault state)

- 0  0  initial state
- 1* 0  assert ok
- 1  1* assert fault
- 1  1  final state
```

When for example both of these groups are asserted, the change in LED state will
be determined by the LED priority. The result is not allowed in the state
diagram in [1].

In this trivial example we can fix it by just assigning Priority "Off" to the
led-blue. But that's not possible at the time of writing (we are on commit
bdbfcde).

There are other examples with more LEDs where that alone will not fix the issue.

### Example ([1] 6.4 PSU Status)

```
LEDs: "AC OK", "FAULT", "LOW V", "BACK UP"
colors: blue,   amber,   amber,   amber
Defined states:
1000 "AC OK"
0100 "AC Fault"
0110 "AC Under Voltage"
0001 "Backup due to AC Outage"

0001 (LED priorities represent "Backup due to AC Outage")

-  0  0  0  0 initial state
-  1* 0  0  0 assert "AC OK"
-  0* 0  0  0 assert "AC Fault" (0100) but LED priority prevented any change.
-  0  0  0  0 final state
```

The final state is undefined according to [1] 6.4.

I did not bother looking at all the different LED priorities possible, since
"Backup..." could very well be the highest priority state.

By now it becomes clear that what's needed is a way to define priorities in
terms of groups and not single LEDs.

## Background and References

[1] OCP Panel Indicator Specification rev1.0

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
- An LED Group / Indicator should not be in an inconsistent state
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

## Alternatives Considered

- extend phosphor-led-sysfs to expose 2 LEDs (blue/amber) as a single LED.

  - This does not work since there may be n different fault LEDs as per [1]
  - Also this is basically lying to ourselves and making it difficult for other
    sw to get any meaningful info about LEDs from dbus anymore.

- Allow Priority "Off" for LED config.
  - This only solves the issue for very simple configurations.
  - individual LED priorities are hard to think about when multiple LEDs form an
    indicator

## Impacts

Need to

- write tests for current assert/de-assert behavior of phosphor-led-manager
- extend docs on how to configure LED group priority
- implement the LED group priority
- perhaps change some configs to use this new feature

There should be no impact on d-bus except if we choose to expose the led group
priority.

### Organizational

- Does this repository require a new repository?
  - No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  - phosphor-led-manager
  - perhaps openbmc to upgrade existing configs
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

How will this be tested? How will this feature impact CI testing?

The feature can easily be unit-tested.
