# Ipmitool-cheatsheet.md

This document is intended to provide a set of ipmitool commands for OpenBMC usage. And this document is copied and pasted for convenience.

## Power

#### 1. View server status

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power status
```

#### 2. Server power on

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power on
```

#### 3. Server power off

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power off
```

#### 4. Server power reset

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power reset
```

#### 5. Server power cycle

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power cycle
```

#### 6. Server power soft

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power soft
```

#### 7. Send a diagnostic interrupt directly to the processor(not support)

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" power diag
```

## Users

#### 1. View information for all users

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" user list
```

#### 2. Display a brief summary of BMC users

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" user summary
```

#### 3. Create a BMC user with a given name

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" user set name <userid> <username>
```

#### 4. Set a given user with a given password

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" user set password <userid>[<password>]
```

#### 5. Disable designated users from accessing BMC

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" user set disable <userid>
```

#### 6. Enable the specified user to access BMC

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" user set enable <userid>
```

##  Field-replaceable Unit (FRU)

#### 1. View FRU information

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" fru list
```

## Sensor Data Record (SDR)

#### 1. View SDR information

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sdr
```

#### 2. Query related SDR information in BMC

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sdr info
```

#### 3. View sensor date records

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sdr list [all|full|compact|event|mcloc|fru|generic]

##
all     : All SDR records (sensors and positioners)
full    : Complete sensor recording
compact : Simple sensor recording
event   : Event information recorded by the sensor
mcloc   : Manage controller locator records
fru     : FRU (Field Replaceable Unit) locator record
generic : General SDR records
```

## Sensors

#### 1. View sensor information

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sensor list
```

## Mc

#### 1. Instruct the BMC to perform a cold reset

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" mc reset cold
```

#### 2. Instruct the BMC to perform a warm reset(not support)

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" mc reset warm
```

#### 3. View BMC information

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" mc info
```

#### 4. View the currently available operation options of BMC

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" mc getenables
```

## Channels

#### 1. Display the authentication function about the selected information channel

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" channel authcap <channel number> <max privilege>
```

#### 2. Display the information for the selected channel

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" channel info [<channel number>]
```

#### 3. View channel information

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" channel info
```

## Chassis

#### 1. Display information about the high-level status of the system rack and power subsystem.

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis status
```

#### 2. The command will return the power on time

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis poh
```

#### 3. Query the reason for the last system restart.(not support)

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis restart_cause
```

#### 4. Set rack power strategy in case of power failure

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis policy

##
policy <state>
list        : return supported policies
always-on   : turn on when power is restored
previous    : return to previous state when power is restored
always-off  : stay off after power is restored
```

#### 5. View and change power status

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis power

##
status : show current status
on     : power on
off    : power off
reset  : power reset
soft   : power soft
cycle  : power cycle
```

#### 6. Ask the system to start the alternate boot device from the system when it restarts next time

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis bootdev <device>

##
Currently supported devices:
pxe    : boot from pxe
disk   : boot from BIOS default boot device
safe   : boot from BIOS default boot device,but requires a safe mode
diag   : boot from the diagnostic partition
cdrom  : boot from the CD/DVD
bios   : enter bios settings
floppy : boot from Floppy/primary removable media
```

#### 7. Control panel logo light

```
$  ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" chassis identify <interval>
```

## Local Area Network(LAN)

#### 1. Output the current configuration information of a given channel

```
 $ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" lan print [<channel number>]
```

#### 2. Set the given parameters for the given channel

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" lan set <channel> <command> <parameter>
##
Valid parameter
ipaddr <x.x.x.x>                 : Set ip for this channel
netmask <x.x.x.x>                : Set netmask for this channel
macaddr<xx:xx:xx:xx:xx:xx>       : Set the mac address for this channel
defgw ipaddr <x.x.x.x>           : Set the default gateway IP address
defgw macaddr<xx:xx:xx:xx:xx:xx> : Set the mac address of the default gateway
bakgw ipaddr <x.x.x.x>           : Set the IP address of the backup gateway
bakgw macaddr<xx:xx:xx:xx:xx:xx> : Set the IP address of the backup gateway
password <pass>                  : Set no user password
access <on|off>                  : Set the LAN channel access mode
```

## System Event Log (SEL)

#### 1. Query the relevant information about SEL and its content in BMC

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sel info
```

#### 2. Clear the information in SEL,but it cannot be undone.

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sel clear
```

#### 3. Delete a single event

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sel delete <number>
```

#### 4. Display the current time of the SEL clock

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sel time get
```

## Session

#### 1. Display session information

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" session info
```

## Serial Over Lan (SOL)

#### 1. Retrieve Serial-Over-LAN configuration information for the specified channel.

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sol info [<channel number>]
```

#### 2. Put ipmitool into Serial Over LAN mode

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sol activate
```

#### 3. Disable serial LAN in BMC mode

```
$ ipmitool -H "$BMC_IP" -I lanplus -U "$BMC_USER" -P "$BMC_PASSWD" sol deactivate
```

