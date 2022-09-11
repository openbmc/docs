# Cable Validation on Aggregated Systems

Author:
  Santosh Puranik (puranik)

Other contributors:
  Jinu Thomas (jinujoy)

Created:
  September 9, 2022

## Problem Description

IBM plans to develop servers consisting of multiple sleds, where each sled is a
self-contained compute unit, but with the capability to connect multiple of them
together using SMP cables. Each sled will have a BMC that is responsible for
managing the sled. Multiple of such sleds in a rack can be used to form one or
more composed systems.

There will be a "system coordinator" (SCA) group of applications that can run on
any one of the sled BMC at a given time. The task of these applications is to
perform management at a "system-level" as opposed to a sled-level.

One such application has the task of performing SMP cable validation. The intent
of cable validation is to ensure that all required SMP cables are plugged in;
that they are the right length for the SMP link; and that the SMP topology is valid.

This document intends to provide a high-level design proposal for the SMP cable
validation application.

## Background and References

This [Redfish aggregation proposal][Redfish-Aggregation] that describes Redfish
aggregation and how that can be used to aggregate data from satellite BMCs.

## Requirements

The following are the high-level requirements for the cable validation
application:

- The application must be run as a part of the SCA startup and validate all
  cables before the BMCs reach a Ready state. It must be re-run when the system
  is powered on
- The application must validate if all the cables required to complete a valid
  SMP topology (which can change depending on the number of sleds belonging to
  the SMP group) are plugged in and detected.
- The application must validate if all the above cables are the right length.
- The application must validate if the right cable is plugged into the right SMP
  port, aka, topology validation.
- The application must provide this data (TBD - via Redfish aggregation?) to an
  external Redfish client/management entity.
- The application must publish this data over a TBD D-Bus API for other SCA
  applications to consume.
- As an additional requirement, the application must be able to guide the user
  through cabling a "bare" system. The details are outside the scope of this
  document, but will involve guiding the user to plug cables in a predetermined
  order by guiding the user via SMP port LEDs.
- The application must log PELs for every cable that it finds
  missing/mis-plugged/of wrong length.

In order to satisfy the above requirements, the following assumptions have been
made:

- The BMC on a sled has the capability to detect cable presence and report this
  via a suitable Redfish schema - such as the Cable/Port schema. The exact
  mechanism of how cable presence is detected is beyond the scope of this
  document.
- The BMC on a sled has the capability to detect cable length and report it via
  Redfish. The exact mechanism of how the length is detected is beyond the scope
  of this document.
- The SCA cable validation application has prior knowledge of the expected SMP
  topology that it needs to validate.
- The SCA application is able to locate the sled positions. This could be done
  by ensuring that the BMC on each sled is able to read their own "sled ID".
- The BMC on each sled is able to control LEDs on SMP ports.

## Proposed Design

The design proposes the following:

Each individual BMC shall read the SMP cable presence and cable length of the
cable (if any) plugged into its SMP ports. This information shall be made
available on D-Bus via existing D-Bus interfaces. The BMCs shall also serve this
information via the Redfish Port and/or Cable schemas. Existing properties like
Status::Sate and PartNumber can be used to represent cable presence and lengths
respectively.

How the application reads the presence/length is an implementation detail, but
it could, for example, be GPIOs that the BMC can read.

- Develop a cable validation daemon that runs as a part of the SCA suite of
  applications.
- The daemon on startup shall read a configuration file that describes the SMP
  topology that it needs to validate. This configuration file must have
  information about the following:

  - A table of all valid cabling topology(-ies) based on the number of sleds
    that form an SMP group (and their positions).
  - The table shall contain a way to uniquely identify the locations of the
    ports. This can, for example, be mapped to the Redfish
    PartLocation::ServiceLabel property. See [this][redfish-location] for more
    details. On D-Bus, this can be mapped to the [LocationCode][dbus-location]
    property.
  - The table shall contain the expected cable lengths for every pair of ports
    that are connected over an SMP cable.
- Upon application startup, the daemon shall parse the configuration and query
  the Port/Cable information from satellite BMCs that belong to the SMP group.
- In order to perform topology validation, the daemon has to write at one end of
  the cable and read from another. This document proposes using GPIOs to do
  this, but the design does not preclude using a more complicated mechanism. In
  order to toggle GPIOs on ports, this document proposes the use of a (possible
  OEM) Redfish property on the Port schema. The property needs to be writeable.
  Setting property should trigger a GPIO write and getting the property a GPIO
  read.
- The validation data shall be made available via the Redfish aggregation
  service. Details are TBD, but this would likely involve the capability of
  supplying the cable status to the satellite BMCs post validation.
- In order to support guided cable installation, the application needs to toggle
  LEDs on Ports of the satellite BMCs. Redfish's LocationIndicatiorActive
  property cannot be used for all cases here as we need to support different LED
  blink rates. This will either need to be OEM or some schema changes are
  needed.

## Alternatives Considered

NA

## Impacts

bmcweb code and each sled needs modifications to support the new properties
being proposed for topology validation. The D-Bus backend for reading/writing
GPIOs is still TBD.

### Organizational

- Does this repository require a new repository?  Yes
- Who will be the initial maintainer(s) of this repository? TBD
- Which repositories are expected to be modified to execute this design? TBD
- Make a list, and add listed repository maintainers to the gerrit review. TBD

## Testing

TBD

## Footnotes

[Redfish-Aggregation]:
    https://gerrit.openbmc.org/c/openbmc/docs/+/44547/2/designs/redfish-aggregation.md
[redfish-location]:
    http://redfish.dmtf.org/schemas/v1/Resource.v1_14_1.json#/definitions/Location
[dbus-location]:
    https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/LocationCode.interface.yaml
