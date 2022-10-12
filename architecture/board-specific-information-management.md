# Board Specific Information Management

There are more than one board in multi-host architecture.

For each board, there are specific board information should be maintained such as

    - Board SKU ID
    - Board revision ID
    - Board config ID
    - Board class type

Services should do specific behavior depended on different board information.

For example, the fan controller service loads corresponding fan table depended on board class type.

So, this document proposes a new design to manage board speific information.


**Block and Interface Diagram**

The below diagram shows the new design for board specific information management.

There is a service called "board-info.service."

During BMC booting, the "board-info" service parses the platform json file.

The service reads the board information through the interface that is defined in the property "Method" of JSON file.

Now, the "board-info" service supports "GPIO," "I2C" and "IPMI" methods for getting the board information.

And then, BMC saves these information on D-bus.

Service(Redfish API, IPMI demean or PID controller) acquires the board information from the D-bus if it needs.


Additional, BMC supports the hot-plugged mechanism for hot-plugging boards through GPIO status.

According to the GPIO pin that is defined in JSON file, the "board-info" service registers a D-bus matcher to monitor GPIO property owned by GPIO monitor service.

BMC queries the board information if GPIO status is triggered.

```
+----------------------------------------------------------------------------------+
| Baseboard                                                                        |
| +---------------------------------------------------------------------+          |
| | BMC                         +----------------------+                |          |
| |                             | GPIO monitor service >----------------+----------+-+
| |                             +-----------^----------+                |          | |(present pins)
| | +-------------+                         |(d-bus matcher)            |          | |     +----------------------+
| | | Redfish API >--+   +------------------^-------------------------+ |          | +----->Server Board          |
| | +-------------+  |   | Service: xyz.openbmc_project.board.info    | |          | |     | +----+-------------+ |
| |                  +---> Object Path:                               >-+--IPMB----+----+--+->BIC |             | |
| |                  |   |   - /xyz/openbmc_project/boardname/nodename| |          | |  |  | |    |     Host    | |
| |                  |   | Interface:                                 | |          | |  |  | +------------------+ |
| |                  |   |   - xyz.openbmc_project.boardname.nodename >-----+      | |  |  +----------------------+
| |                  |   | Properties:                                | |   |      | |  |           .
| |                  |   |   - property                               | |  I2C     | |  |           .
| | +----------+     |   +------^-------------------------------------+ |   |      | |  |           .
| | |   IPMI   >-----+          |                                       | +-v----+ | |  |           .
| | +----------+       +--------^-------------+                         | | CPLD | | |  |  +----------------------+
| |                    | phosphor-pid-control |                         | +------+ | +----->Server Board          |
| |                    +----------------------+                         |          |    |  | +----+-------------+ |
| |                                                                     |          |    +--+->BIC |             | |
| +---------------------------------------------------------------------+          |       | |    |     Host    | |
|                                                                                  |       | +------------------+ |
+----------------------------------------------------------------------------------+       +----------------------+
```

**JSON Requirements**

The information should be maintained is listed in JSON file.

The JSON format is 
```
[
    {
        "Name": "boardname",
        "Nodes": [
            {
                "Name": "nodename",
                "Properties": [
                    {
                        "Name": "propertyname1",
                        "Method": "gpio",
                        "Labels": {
                            "ChipId": "${CHIP_ID}",
                            "GpioNum": "${GPIO_Number}"
                        }
                    },
                    {
                        "Name": "propertyname2",
                        "Method": "i2c",
                        "Labels":{
                            "BusId": "${I2C_BUS}",
                            "Address": "${7_BIT_SLAVE_ADDRESS}",
                            "Offset": "${REGISTER}"
                        }
                    },
                    {
                        "Name": "propertyname3",
                        "Method": "ipmi",
                        "Labels":{
                                "NetFn": "${NETFN}",
                                "Cmd": "${COMMAND}",
                                "ReqData": "${REQUEST_DATA}"
                        }
                    }
                ],
                "HotPlugged": {
                    "Method": "gpio",
                    "Labels": {
                            "Name": "${GPIO_Name}",
                            "TriggerEdge": "low"
                    }
                }
            }
        ]
    }
]
```

**Dbus Interface**

```
root@greatlakes:~# busctl introspect xyz.openbmc_project.board.info /xyz/openbmc_project/boardname/nodename
NAME                                   TYPE      SIGNATURE RESULT/VALUE FLAGS
org.freedesktop.DBus.Introspectable    interface -         -            -
.Introspect                            method    -         s            -
org.freedesktop.DBus.Peer              interface -         -            -
.GetMachineId                          method    -         s            -
.Ping                                  method    -         -            -
org.freedesktop.DBus.Properties        interface -         -            -
.Get                                   method    ss        v            -
.GetAll                                method    s         a{sv}        -
.Set                                   method    ssv       -            -
.PropertiesChanged                     signal    sa{sv}as  -            -
xyz.openbmc_project.boardname.nodename interface -         -            -
.propertyname1                         property  y         18           emits-change
.propertyname2                         property  y         18           emits-change
.propertyname3                         property  y         18           emits-change
```
