# Ipmitool cheat sheet

This document is intended to provide a set of ipmitool commands for OpenBMC usage. 

## power

#### 1. View server status

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power status
```

#### 2. Server power on

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power on/up
```

#### 3. Server power off

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power off/down
```

#### 4. Server power reset

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power reset
```

#### 5. Server power cycle

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power cycle
```

#### 6. Server power soft 

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power soft
```

#### 7. Send a diagnostic interrupt directly to the processor(not support)

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) power diag
```

## user

#### 1. View information for all users

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) user list
```

#### 2. Display a brief summary of user id information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) user summary
```

#### 3. Set the given user id to the given user name

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) user set name <userid> <username> 
```

#### 4. Set a given user with a given password

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) user set password <userid>[<password>]
```

#### 5. Disable designated users from accessing BMC

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) user set disable <userid>
```

#### 6. Enable the specified user to access BMC

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) user set enable <userid>
```

##  fru

#### 1. View fru information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) fru list
```

## sdr 

#### 1. View sdr information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sdr
```

## sensor

#### 1. View sensor information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sensor list
```

## mc

#### 1. Instruct the BMC to perform a warm or cold to be reset.

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) mc reset <warm|cold> 
```

#### 2. View BMC firmware information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) mc info
```

#### 3. View the currently available operation options of BMC

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) mc getenables
```

## channel

#### 1. Display the authentication function about the selected information channel

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) channel authcap   <channel number> <max privilege>
```

#### 2. Display the information of the selected channel

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) channel info      [channel number]
```

#### 3. View channel information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) channel info
```

## chassis

#### 1. Display information about the high-level status of the system rack and power subsystem.

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis status
```

#### 2. The command will return the poweron time

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis poh
```

#### 3. Query the reason for the last system restart.(not support)

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis restart_cause
```

#### 4. Set rack power strategy in case of power failure

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis policy
```

#### 5. View and change power status

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis power

##
status: show current status
on: power on     
off: power off
reset: power reset
soft:  power soft
cycle: 
```

#### 6. Ask the system to start the alternate boot device from the system when it restarts next time

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis bootdev <device>

##
Currently supported devices:
pxe: boot from pxe
disk: boot from BIOS default boot device
safe: boot from BIOS default boot device,but requires a safe mode
diag: boot from the diagnostic partition
cdrom: boot from the CD/DVD 
bios: enter bios settings
floppy: boot from Floppy/primary removable media
```

#### 7. Control panel logo light (not support)

```
$  ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) chassis identify <interval>
```

## lan

#### 1. Output the current configuration information of a given channel

```
 $ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) print [<channel number>]
```

#### 2. Set the given parameters for the given channel

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) lan set <channel> <command> <parameter>
```

## sdr

#### 1. Query related SDR information in BMC

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sdr info
```

#### 2. View sensor date records

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sdr list [all|full|compact|event|mcloc|fru|generic]

##
all: All SDR records (sensors and positioners)
full: Complete sensor recording
compact: Simple sensor recording
event: Event information recorded by the sensor
mcloc: Manage controller locator records
fru: FRU (Field Replaceable Unit) locator record
generic: General SDR records
```

## sel

#### 1. Query the relevant information about sel and its content in BMC

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sel info
```

#### 2. Clear the information in sel ,but it cannot be undone.

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sel clear
```

#### 3. Display the first count information in sel.If count is 0, then all information will be displayed

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sel <count>|first <count>
```

#### 4. Display the first count information in sel.If count is 0, then all information will be displayed

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sel last <count>
```

#### 5. Delete a single event

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sel delete <number>
```

#### 6. Display the current time of the SEL clock

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sel time get
```

## pef

#### 1.  Query the BMC and print out information about the functions supported by PEF

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) pef info
```

#### 2. Print out the current status of pef

$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) pef status

#### 3. Lists PEF strategy table entries

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) pef policy
```

#### 4. List PEF entries(not support)

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) pef list
```

## session

#### 1. Display session information

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) session info
```

## sol

#### 1. Retrieve Serial-Over-LAN configuration information for the specified channel.

```
$ ipmitool -H xxx(BMC management IP address) -I lanplus -U xxx(BMC login username) -P xxx(BMC login password) sol info [<channel number>]
```

