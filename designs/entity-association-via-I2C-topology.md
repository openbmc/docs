# Platform physical model : association between entities

Author: Jie Yang (jjy)

Created: June 10, 2021

## Problem Description

Current [Entity Manager](https://github.com/openbmc/entity-manager) does not
support to detect the physical connectivity between entities. But platform
management software is interested in how an entity is assembled into the
platform, when the entity is detected. For example, an expansion card is
inserted on the PCIE slot labeled "PE0" of the baseboard. This connectivity
information is required when building the platform physical model.

## Background

Many platforms can have multiple chassis, trays, risers, drives, et al. that own
entity configuration in Entity Manager. Platform management software shall
collect the inter-connectivity information between entities via Redfish. With
such connectivity info platform management software can derive the relative
connectivity path of an entity to the baseboard that is where the entity locates
and then report the location of any active entities.

## Requirements
1. The connectivity info should be exposed to DBus interface. If there is a
cable that connects two entities, cable info should also be present.

2. The connectivity info would be dynamical, when an entity can be plugged into
different slots (e.g. a PCIE device might be swapped from PCIE slot 0 to PCIE
slot 1). Entity manager should detect such connectivity change.

## Solution

### Expose connector as device

All connectors like PCIE slots, riser connectors and NIC connectors that have
I2C bus pass-through will be exposed as devices in the entity configuration with
an abstract type `I2CConnector` and a property `Location` that is a free form,
implementation defined string to provide the connector location on the entity.
In our platforms, each connector has a silkscreen label that would be hard-coded
as the `Location` property of the slot in the entity configuration. Connectors
will also have an `Bus` property that is the I2C bus going into the connector.
The bus number can be a physical I2C bus on the baseboard that may need
hard-coding in the entity configuration or a logical bus behind an I2C mux
device that will be determined when the mux driver is loaded.

Entity manger will publish each connector with a DBus object
`/xyz/openbmc_project/Inventory/Item/{Entity Type}/{Entity Name}/{ConnectorName}
`. The connector object path will have a DBus interface
`xyz.openbmc_project.Configuration.I2CConnector` with properties
`Bus`, `Name`, `Location` and `Type`. When Entity Manager scan the entity
config, a map between I2C bus and `I2CConnector` can be built.
Using openbmc object mapper, we can build
a map between I2C buses and connector object paths, which is supposed to be a
1:1 mapping. Note that there is an underlying assumption that is an I2C bus only
goes to one entity connector.

### Associate entities

When an entity EEPROM is probed, Entity Manager will create a `Bus` property for
the entity that is the I2C bus number of the probed entity EEPROM. If this I2C
bus is also mapped to a `I2CConnector` on a different entity in the built
I2C:`I2CConnector` map, there should be a connectivity between those two
entities. With all the information described above, we can build association
between all active entities in the system. The association forward and reverse
types for board entities are defined as `containedby` and `contains` that would
conform to Redfish Chassis schema.

Note that it is also possible that an I2C bus can be mapped to multiple
`I2CConnector`, when an I2C bus passes through multiple boards. As an
illustration, assuming that the following IO Riser entity and NIC entity EEPROM
are both on I2C bus 2. From the entity configuration, I2C bus 2 is mapped to two
connectors: PE0 on the motherboard and IO1 socket on the IO Riser. When building
the connectivity graph, there would be an redundant edge between motherboard and
NIC.  In this case, a simple BFS traversal can trim the redundant edge. Using
such approach, the IO Riser will have a connectivity path /root/PE0 and NIC will
have /root/PE0/IO1. As PE0 and IO1 are printed silkscreen label on the board in
our system, lab engineer can easily identify the entity.

+-------------+              +----------+            +-----+
|             |  i2c-2: PE0  |          |  i2c-2:IO1 |     |
| Motherboard |------------->| IO Riser |----------->| NIC |
|             |              |          |            |     |
+-------------+              +----------+            +-----+

When creating association, there would be two scenarios.
1. The downstream entity EEPROM locates on an I2C bus that is also mapped to a
connector object. Then directly associate the downstream entity with the
upstream entity on which the connector locates.
2. The I2C bus of the downstream entity EEPROM is not in the I2C and connector
objects map. In this case, we will need to find the I2C bus on the upstream
entity that passes through a connector on the upstream entity and enters to the
downstream entity. When the downstream entity EEPROM is behind an I2C mux, the
I2C bus of downstream entity EEPROM would be different from that of upstream
entity connector. Thus we will need to trace the parent I2C bus in the front of
I2C mux recursively by iterating the system I2C bus directory until a connector
on the upstream entity is found. Then we can create the association. If we
cannot find a connector, which means the entity is the root entity -
motherboard. No need to build such association to the motherboard.

### Export physical connectivity to bmcweb

Implement entity Redfish resource with ["Links"]["Contains"] and
["Links"]["ContainedBy"] entries by looking for associations with names
"contains" and "containedby" that determine the downstream entities and upstream
entity that are connected to the entity.
