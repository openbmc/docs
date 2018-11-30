# Host collecting thermal zone config information from BMC

Author:
  [Jaghathiswari Rankappagounder Natarajan](mailto:jaghu@google.com), jaghu

Primary assignee:
  jaghu

Other contributors:
  dchanman 

Created:
  30 November, 2018

## Problem Description
Develop a collection system that allows Host to fetch thermal zone config
information from BMC.

## Background and References
A system can have one or more cooling zones. Each zone will contain at least
one fan and some physical entities. In each zone, there could be physical
entities whose temperature/margin sensor values only Host can read and some
which only the BMC can read. Host has the ability to send to the BMC (through
an IPMI OEM command), the margin information on the devices that it knows how
to read that the BMC cannot. The state of the BMC readable temperature sensors
can be read through normal IPMI commands and is already supported. The BMC will
run a daemon (phosphor-pid-control) that controls the fans by predefined zones.
The application will use margin-based thermal control, such that each defined
zone is kept within a range and adjusted based on thermal information provided
from locally readable sensors as well as Host provided information.

## Requirements
Host will know about the physical entities (like DIMM, CPU, etc) that it needs
to monitor in a particular zone and after fetching all the temperature/margin
sensor values for those entities, Host will calculate the worst case fleeting
margin sensor value. The worst case fleeting margin sensor value is the lowest
value out of all the fleeting margin sensor values. Fleeting margin sensor
value tends to indicate the margin sensor value of a device whose temperature
tends to change rapidly. Host will use an IPMI OEM command to write the worst
case fleeting margin sensor value to the fleeting margin sensor for that zone.
BMC will then use this value in its margin-based thermal control.

There are two pieces of thermal zone config information which are hard-coded in
Host:

**What is the fleeting margin sensor ID that Host needs to write to on
the BMC side?**
For example, there is a zone called “central” and the fleeting margin sensor ID
on the BMC side is 208.
```
thermal_zone {
      name: "central"
      host_sensor_ids {
        fleeting_margin: 208
      }
}
```

**What are all the physical entities that Host needs to monitor in a particular
zone?**
For example, there is a zone called “central” and Host needs to monitor
all the devices on the motherboard.
```
thermal_zone {
  name: "central"
  board_level: true
  controlling_device_devpath: "BMC_RISER_CONN/bmc_riser/open_bmc"
}
```
This design proposal is to move all this information to BMC and facilitate Host
to automatically fetch this information from BMC.

## Proposed Design
In case of OpenBMC, the Host model is that, each zone could
have several physical entities and one fleeting margin sensor.

### What information should be added to OpenBMC:
Say for example, there is one cooling zone 1 which monitors 6 DIMMs, 1 fan and
has one fleeting margin sensor.

#### Entity ID and Entity Instance values:
The Sensor Data Records for all the fleeting margin sensors (for each zone)
should have the proper Entity ID and Entity Instance values. For more
information on SDRs, one can reference section 43.1 in the IPMI spec. The
Entity ID indicates the physical entity the sensor is monitoring or is
otherwise associated with the sensor and Entity Instance is the instance number
for that entity. Entity Instance could be device relative or system relative.
As per the Entity Id codes in the IPMI spec (Section 43.4), the Entity ID for a
cooling zone is 1Eh. For example, the fleeting margin sensor associated with
the cooling zone 1 above should have Entity ID as 1Eh and the Entity Instance
could be 01h (since there is only one cooling zone).

#### Entity Association records:
Entity Association records are a special type of SDR that provides a definition
of the relationships among platform entities.  For more information on Entity
Association Records, one can reference section 43.4 in the IPMI spec. For
example, the Entity Association Record for the above cooling zone 1 would be
like:

|Entity ID|Entity Instance|Entity Instances| Description|
|--------|-------|----|-------|
|0x1E|0x01|1Dh, 01h and 20h,01h -> 20h,06h| Cooling Zone 1: Fan 1, DIMM1-6|

#### Entity ID to silk screen name mapping info:
For a given Entity ID and Entity Instance value, we would like to know about
the silk screen name/standard name of the entity. We can store this information
in a JSON file in a format like ```Entity ID.Entity Instance:Entity name```.
For example, the JSON file could be like this:
32.1:DIMM1
32.2:DIMM2
32.3:DIMM3
32.4:DIMM4
32.5:DIMM5
32.6:DIMM6
16.1:PCIE1
16.2:PCIE2
16.3:PCIE3
43.1:CPU1
43.2:CPU2

#### IPMI OEM command to retrieve Entity ID to silk screen name mapping info:
A Host Google OEM IPMI Command byte sequence is as follows.  The payload
contents vary by command.

Request
root@bmc:~# ipmitool -I dbus raw 0x2e 0x32 0x79 0x2b 0x00 0x00 0x04 0x65 0x74
0x68 0x31

[0] 0x2e (OEM IPMI Command)
[1] 0x32 Custom Command
[2] 0x79 (OEM Number 3-bytes)
[3] 0x2b
[4] 0x00
[5..N] Payload

Response
 79 2b 00 00 2f 36 5b 08 00 00 00 00

[0] 0x79 (OEM Number Byte)
[1] 0x2b (OEM Number Byte)
[2] 0x00 (OEM Number Byte)
[3..N] Payload

**GetEntityIDName - SubCommand 0x06**
Host can get the Entity ID to name mapping from BMC using this command. When
BMC receives this command, BMC can check the related JSON file and then
send the name for that particular entity this command response.
Request
|Byte(s) |Value  |Data
|--------|-------|----
|0x00|0x06|Subcommand
|0x01|Entity ID|Entity ID
|0x02|Entity Instance|Entity Instance

Response
|Byte(s) |Value  |Data
|--------|-------|----
|0x00|0x06|Subcommand
|0x01|Entity name length (say N)|Entity name length
|0x02...0x02 + N - 1|Entity name|Entity name without null terminator

Examples
Host requests for the entity name for a PCIe slot(Add-in card).
ipmitool -I dbus raw 0x2e 0x32 0x79 0x2b 0x00 0x06 0x0b 0x01
 79 2b 00 06 05 73 6c 6f 74 35

### How can Host fetch thermal zone config info from BMC:

**Step 1:** Iterate through all SDRs and find only SDRs that are of sensor type
temperature and is settable. We should look for Full Sensor Records (SDR type
01h). One can look at Section 43.1 in the IPMI spec to find out more about Full
Sensor Records. To find out the sensor type, we have to look at “Sensor Type”
byte in the SDR. To find out if the sensor is settable, we have to look at
“Sensor Initialization” byte in the SDR.

For each SDR found:

**Step 1a:** Find the Entity ID for that sensor. To find out the Entity ID, we
have to look at “Entity ID” byte in the SDR.

**Step 2a:** If the entity is a cooling zone(Entity ID code : 1Eh), then fetch
the Entity Association Record for that entity. Find out all the contained
entities inside that zone. The Entity Association Record will give the Entity
IDs for all the contained entities. Send OEM IPMI commands from Host to get the
Entity ID to silk screen name mapping. For a particular zone, now Host has the
information about what entities it should monitor and about the fleeting margin
sensor (sensor with the SDR found above) in that zone. Add this information to
thermal zone config file in the Host.

## Alternatives Considered
One alternative considered was a temperature/margin sensor associated with each
device (that only Host can read) could be considered to be in separate zone on
the Host side, but the downside is that there will lot of IPMI write overhead.

## Impacts
We are making this collection system automatic. This reduces development time
spent to add relevant information in both Host and BMC. Now we need to add
information only in BMC and Host will fetch it automatically.

## Testing
In Host, write end-to-end test to check if we got all the relevant thermal zone
config information.  In BMC, write unit tests to check if all relevant thermal
zone config information is added correctly.
