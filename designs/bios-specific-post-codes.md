# BIOS specific Post Codes in Redfish

Author:
  Manojkiran Eda

Primary assignee:
  Manojkiran Eda

Other contributors:
  None

Created: March 26, 2021

## Problem Description

Currently the BIOS Progress codes aka Post Codes are exposed via dbus internally
& leverages the redfish PostCode LogService to push them out of BMC to all the
external redfish clients. But the existing framework works only with hex based
integer post codes & lacks in having system specific/ BIOS specific/ processor
specific post codes in redfish. This document proposes a few changes in back-end
(post-code-manager) & also the BMCWeb to support modeling the system specific
postcodes.

## Background and References

Currently, OpenBMC exposes BIOS POST codes on Dbus using the
`xyz.openbmc_project.State.Boot.PostCode service`. The existing interface tracks
POST codes with their time-stamps for the past `100` host boot events and the
current boot cycle index. BMCWeb uses the existing dbus method
`GetPostCodesWithTimeStamp` to obtain all the Post codes on the system with
micro-second resolution.

```
- name: GetPostCodesWithTimeStamp
  description: >
      Method to get the cached post codes of the indicated boot cycle with time-stamp.
  parameters:
    - name: Index
      type: uint16
  returns:
    - name: Codes
      type: dict[uint64, struct[uint64,array[byte]]]
      description: >
        An array of post codes and timestamp in microseconds since epoch
```
The result of the dbus API method call to `GetPostCodesWithTimeStamp` will look
like below :

```
{
  "call": "GetPostCodesWithTimeStamp",
  "interface": "xyz.openbmc_project.State.Boot.PostCode",
  "obj": "/xyz/openbmc_project/State/Boot/PostCode",
  "result": [
    {"20191223T143052.632591", 1},
    {"20191223T143052.634083", 2},
    {"20191223T143053.928719", 3},
    {"20191223T143053.930168", 4},
    {"20191223T143054.512488", 5},
    {"20191223T143054.513945", 6},
    {"20191223T143054.960246", 17},
    {"20191223T143054.961723", 50},
    {"20191223T143055.368219", 4},
    {"20191223T143055.369680", 173}
  ],
  "status": "ok"
}
```

## Requirements

Here are the following requirements & goals that are addressed as part of this
design document:
1. Allow a client to distinguish between postcodes that are ASCII based versus
   the post codes that are not.
2. A solution that should not break the existing redfish client or expect them to
   change their code.
3. A Scalable solution that should work even when we want to support a new format
   of post codes in future.
4. Redfish Interface should be Redfish compliant and pass the Redfish compliance
   tests.

## Proposed Design

The existing dbus method `GetPostCodesWithTimeStamp` always returns map with
`{time-stamp : progress-code-structure}`, where the progress-code-structure is
always an Unsigned Integer.

Instead , the proposal is to add a new dbus property `PostCodeFormat` of enumeration
type `xyz.openbmc_project.State.Boot.PostCode` interface that would act as a
placeholder for holding which type of Progress Codes does the system/bios want, as
shown below:

```
properties:
    - name: PostCodeFormat
      type: string
      description: >
        The Postcode Format identifier.
    - name: Purpose
      type: enum[self.PostCodeType]
      description: >
        The format of the post codes.  As in, to what type should we decode them
        postcode into ?
enumerations:
    - name: PostCodeType
      description: >
        An enumeration of possible interpretations of postcodes.
      values:
        - name: Integer
          description: >
            Used to interpret a Post code as an Unsigned Integer
        - name: ASCII
          description: >
            Used to interpret a Post Code as an ASCII string.
```

This dbus property `PostCodeFormat` should be set via a `meson/cmake option`
from the vendor specific layers(meta-vendor).

The PostCode manager daemon should be able to return the Post Codes in the format
that is set by the `PostCodeFormat` dbus property. To achieve this, we need to
add a new dbus method which can return postcodes in various formats. Say for
example, if the `PostCodeFormat` dbus property is set to ASCII, then the new method
`GetPostCodesWithTimeStampByFormat` should return some thing like below:
```
{
  "call": "GetPostCodesWithTimeStampByFormat",
  "interface": "xyz.openbmc_project.State.Boot.PostCode",
  "obj": "/xyz/openbmc_project/State/Boot/PostCode",
  "result": [
    {"20191223T143052.632591", "C2001000"},
    {"20191223T143052.634083", "C2002300"},
    {"20191223T143053.928719", "STANDBY "},
    {"20191223T143053.930168", "C20043FF"},
    {"20191223T143054.512488", "C2004301"},
    {"20191223T143054.513945", "C2006020"},
    {"20191223T143054.960246", "C200XXXX"},
    {"20191223T143054.961723", "D9002770"},
    {"20191223T143055.368219", "RUNTIME "},
  ],
  "status": "ok"
}
```
For example, if the dbus property is set to Integer, then it should return the
Integer post codes as shown below:

```
{
  "call": "GetPostCodesWithTimeStampByFormat",
  "interface": "xyz.openbmc_project.State.Boot.PostCode",
  "obj": "/xyz/openbmc_project/State/Boot/PostCode",
  "result": [
    {"20191223T143052.632591", 1},
    {"20191223T143052.634083", 2},
    {"20191223T143053.928719", 3},
    {"20191223T143053.930168", 4},
    {"20191223T143054.512488", 5},
    {"20191223T143054.513945", 6},
    {"20191223T143054.960246", 17},
    {"20191223T143054.961723", 50},
    {"20191223T143055.368219", 4},
    {"20191223T143055.369680", 173}
  ],
  "status": "ok"
}
```

BMCWeb should host different message registries for different possible post code
formats supported by the dbus Property `PostCodeFormat`.As an example below are
few message registries for both Integer & ASCII post codes.

```
MessageEntry{"BIOSPOSTCode",
             {
                 "BIOS Power-On Self-Test Code received",
                 "Boot Count: %1: TS Offset: %2; POST Code: %3",
                 "OK",
                 "OK",
                 3,
                 {"number", "number", "number"},
                 "None.",
             }},

MessageEntry{"BIOSPOSTCodeASCII",
             {
                "BIOS Power-On Self-Test Code received",
                "Boot Count: %1: TS Offset: %2; POST Code: %3",
                "OK",
                "OK",
                 3,
                {"number", "number", "string"},
                "None.",
            }},

```

And BMCWeb would have to check for the `PostCodeFormat` dbus property and based
on that, it should switch the registries.

In future, if any vendor comes up with a new post code format, they should add it
to the `PostCodeFormat` enumeration , add the necessary support to the
post-code-manager and add a new message registry in BMCWeb.

## Alternatives Considered

The simple alternative design is to just do the conversion from integer to the
desired format with in BMCWeb & also enhance the existing `BIOSPOSTCode` message
registry to contain placeholders for all the different formats like Integer and
ASCII.

This approach is not scalable & also, we display all post code formats in all the
systems irrespective of BIOS/System/Processor which is is not a great solution.

## Impacts
Backward compatibility remains with the existing DBUS interface method. Minimal
performance impact is expected to do the conversion from Integer to expected
formats.

## Testing
Compliance with Redfish will be tested using the Redfish Service Validator.
