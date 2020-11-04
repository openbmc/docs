# Multi-host Bios Update Support

Author: Priyatharshan P, [priyatharshanp@hcl.com]

Other contributors: None

Created: Nov 04,2020

## Problem Description

The current implementation in the phosphor-bmc-code-mgmt supports only single
host bios update.

As the open BMC architecture is evolving, the single-host-bios-update support
becomes contingent and needs multi-host-bios-update to be implemented for the
multi-host environment.

## Requirements
The multi-host bios update implementation considers the following requirements.
- Loading single bios image into multiple host as applicable.
- User to configure the list of hosts needing upgrade.
- The bios image to be deleted after updating all the hosts configured for
  upgrade.

## Proposed Design
This document proposes a new design engaging a phosphor-bmc-code-mgmt to update
bios image for multi host system. Provided below the implementation steps.
- phosphor-dbus-interfaces changes
    - Create a new biosUpdate interface
- phosphor-bmc-code-mgmt changes
    - Host Identification
    - New hostX objects creation
    - Bios image clean up
    - Property reset in biosUpdate interface
- Multi-host bios update flow
- Redfish & busctl API

### Create a new biosUpdate interface:
In the existing single host system, the bios update command will be called
directly for the bios update and the image will be deleted after successful
update.

But in the multi host system, before bios update, user needs to specify the
list of hosts which requires bios update. As multi-host support is a new
implementation, there is no property to hold which host to be updated.
Hence, creating a new interface through phosphor-dbus-interfaces with the
property “biosUpdate” as shown below to notify the bmc to trigger the bios
update for the configured hosts.
```
properties:
    - name: biosUpdate
      type: boolean
      default: false
      description: >
        The host identifier for bios update.
```
When this Boolean flag is set to TRUE for a host, the bios of the respective
host will be updated.

### Host Identification
Number of hosts presence on the bmc will be identified through the
machine layer (OBMC_HOST_INSTANCES)

N = OBMC_HOST_INSTANCES

### New HostX objects creation

One object per host (/xyz/openbmc_project/software/hostX (Where X = 1,2,...N))
will be created with the new interface.

For Example, In a multi host system [4 hosts] the objects will look like
below:
```
root@yosemitev2:~# busctl tree xyz.openbmc_project.Software.BMC.Updater
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/software
      |-/xyz/openbmc_project/software/a81bb606
      |-/xyz/openbmc_project/software/host1
      |-/xyz/openbmc_project/software/host2
      |-/xyz/openbmc_project/software/host3
      `-/xyz/openbmc_project/software/host4
 ```

Before bios update, the user should set the property biosUpdate to "true"
for all the host which requires update. The interface would look like,
```
root@yosemitev2:~# busctl introspect xyz.openbmc_project.Software.BMC.Updater
/xyz/openbmc_project/software/host1
NAME                                         TYPE      SIGNATURE RESULT FLAGS
org.freedesktop.DBus.Introspectable          interface -         -      -
.Introspect                                  method    -         s      -
org.freedesktop.DBus.Peer                    interface -         -      -
.GetMachineId                                method    -         s      -
.Ping                                        method    -         -      -
org.freedesktop.DBus.Properties              interface -         -      -
.Get                                         method    ss        v      -
.GetAll                                      method    s         a{sv}  -
.Set                                         method    ssv       -      -
.PropertiesChanged                           signal    sa{sv}as  -      -
xyz.openbmc_project.Software.HostBiosUpdate interface -         -       -
.biosUpdate                             property  b         false emits-change
```

### Bios image clean up
In single host system, the bios-image will be deleted after the successful
firmware update. But in multi-host system, the image has to be deleted only
after successfully updating all the hosts having "biosUpdate" property set.

The image delete logic in the existing will be updated to account “biosUpdate”
property of all the host system. When this flag is false for all the
configured host, the image will be deleted at the end.

### Property reset in biosUpdate interface
After successful completion, the property "biosUpdate" will  be reset back to
default value in the hostX [X = 1, 2, …N] objects.

### Multi-host bios update flow
If the user wants to update the same image for host1 and host3 then:

-   User sets the new "biosUpdate" property for host1 and host3 to TRUE.
-   User calls the Redfish/busctl API and select the bios image to upload.
-   The BMC updater calls the bios_X updater script for only the host
    instances having TRUE in the "biosUpdate" property.
-   The bios-image will be deleted after successfully updating all the hosts.
-   The property "biosUpdate" will be reset to default after the successful
    completion of bios-update.

### Assumption
User to know host and slot mapping details to trigger the bios update for
the required  host.

### Redfish  & busctl API commands
In mullti-host system, before calling the bios-update API, the set API
command needs to be called to set "biosUpdate" property to TRUE for each host.

#### Using busctl
To configure the host for bios update,
For Host1:
```
busctl set-property xyz.openbmc_project.Software.BMC.Updater 
/xyz/openbmc_project/software/host1 xyz.openbmc_project.Software.biosUpdate
biosUpdate b true
```
For Host3:
```
busctl set-property xyz.openbmc_project.Software.BMC.Updater
/xyz/openbmc_project/software/host3 xyz.openbmc_project.Software.biosUpdate
biosUpdate b true
```
To update the bios image,
```
busctl set-property xyz.openbmc_project.Software.BMC.Updater
/xyz/openbmc_project/software/<id> xyz.openbmc_project.Software
.Activation RequestedActivation s xyz.openbmc_project.Software
.Activation.RequestedActivations.Active
```
#### Using Redfish:
Following commands could possibly be considered for the redfish support.

To configure the host for bios update,
For Host1:
```
curl -g -k -H "X-Auth-Token: $token"
https://${bmc}/redfish/v1/Host/Actions/host1.BiosUpdate -d '{"Value": "true"}'
```
For Host3:
```
curl -g -k -H "X-Auth-Token: $token"
https://${bmc}/redfish/v1/Host/Actions/host3.BiosUpdate -d '{"Value": "true"}'
```
To update the bios image,
```
curl -u root:0penBmc -curl k -s  -H "Content-Type: application/octet-stream"
-X POST -T <image file path> https://${BMC}/redfish/v1/UpdateService
```

## Alternatives Considered

### Design 1: Multi Interface Approach

1. Number of host will be identified from machine layer [OBMC_HOST_INSTANCES]

2. Code will be modified to create n number of objects based on number of
hosts like below

```
      |-/xyz/openbmc_project/software/host1
      | `-/xyz/openbmc_project/software/host1/28bd62d9
      |-/xyz/openbmc_project/software/host2
      | `-/xyz/openbmc_project/software/host2/28bd62d9
      |-/xyz/openbmc_project/software/host3
      | `-/xyz/openbmc_project/software/host3/28bd62d9
      `-/xyz/openbmc_project/software/host4
        `-/xyz/openbmc_project/software/host4/28bd62d9
```

3. This will create activation interface for each host. For a multi-host
system if the  RequestedActivation is set to
 "xyz.openbmc_project.Software.Activation.RequestedActivations.Active",
then different bios service file will be called based the host.

For single host :
biosServiceFile = "obmc-flash-host-bios@" + versionId + ".service";
For multi host  :
biosServiceFile =  "obmc-flash-host" + host + "-bios@" + versionId +
".service";

Then it can be used for multi host even if the firmware image we want to
install is the same for multiple host targets.

#### Disadvantages

Even after the successful activation of bios update, the image will not be
deleted from the BMC.We dont want to potentially fill up the /tmp space.

### Design 2 : MANIFEST File Changes Approach

Host id will be added as an array of host id in extra version field in the
MANIFEST file.Format can be defined, this way as soon as we see bios image
uploaded and based on extra version, it will create as many version
interface and allow to activate them. Once we call all the bios upgrade
script, we should delete image.

#### Disadvantages

The MANIFEST file is an  factory ship-out, it may not be appropriate to add
the host IDs into it.

## Impacts
None

## Testing
meta-yosemitev2 is a multi-host system [4 host]. This feature will be
tested in meta-yosemitev2 platform.

