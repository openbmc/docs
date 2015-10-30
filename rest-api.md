# OpenBMC REST API
## HTTP GET operations
List directory:
```
curl http://bmc/<path>/
```
Examples:
```
curl http://bmc/
curl http://bmc/org/
curl http://bmc/org/openbmc/
curl http://bmc/org/openbmc/inventory/
curl http://bmc/org/openbmc/inventory/system/chassis/
```

List objects recursively:
```
curl http://bmc/<path>/list
```
Examples:
```
curl http://bmc/list
curl http://bmc/org/openbmc/inventory/list
```

Enumerate objects recursively:
```
curl http://bmc/<path>/enumerate
```
Examples:
```
curl http://bmc/enumerate
curl http://bmc/org/openbmc/inventory/enumerate

```

Get object:
```
curl http://bmc/<path>
```
Examples:
```
curl http://bmc/org/openbmc/inventory/system/chassis/fan2
```

Get property:
```
curl http://bmc/<path>/attr/<attr>
curl http://bmc/org/openbmc/inventory/system/chassis/fan2/attr/is_fru
```

## HTTP PUT operations
These require a json formatted payload.  To get an example of what that looks like:
```
curl http://bmc/org/openbmc/flash/Bios_0 > bios.json # - or -
curl http://bmc/org/openbmc/flash/Bios_0/attr/flasher_path > flasher_path.json
```

A put operation on an object requires a complete object.  For partial updates see POST.

Make changes to the file and do a put (upload):

```
curl -T bios.json http://bmc/org/openbmc/flash/Bios_0
curl -T flasher_path.json http://bmc/org/openbmc/flash/Bios_0/attr/flasher_path
```

## HTTP POST operations
These also require a json formatted payload.

To make a partial change to an object, try removing some properties from the bios.json example above.  Then do:

```
curl -d "`cat bios.partial.json`" http://bmc/org/openbmc/flash/Bios_0
```

To invoke a method:
```
curl -d "[ \"foo\", \"bar\" ]" http://bmc/org/openbmc/managers/System/action/getObjectFromId
```

