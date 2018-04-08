# Request for Comments: OBMC Service Interface
Various services provided by OBMC will have the need to expose methods to
configure, enable, disable and start, stop, reset the service. It is hence
desirable for a generic interface which has the above methods in its namespace.

Each of the service will then implement one or more methods of this interface
as applicable. If Enable and Start are semantically equivalent to the service,
then only one of them may be implemented. Similar is the case with the Disable
and Stop methods.

## org.obmc.Service Interface

### Methods
```
  <interface name="org.openbmc.Service">
    <method name="Enable">
      <arg direction="in"  type="a{sv}" name="argv_dict" />
      <arg direction="out" type="x" />
    </method>
    <method name="Disable">
      <arg direction="in"  type="as" name="argv_list" />
      <arg direction="out" type="x" />
    </method>
    <method name="Reset">
      <arg direction="in"  type="" name="" />
      <arg direction="out" type="x" />
    </method>
    <method name="Start">
      <arg direction="in"  type="" name="" />
      <arg direction="out" type="x" />
    </method>
    <method name="Stop">
      <arg direction="in"  type="" name="" />
      <arg direction="out" type="x" />
    </method>
  </interface>
```

### Description
#### Enable method
```
Enable (IN a{sv} argv_dict,
    OUT x return_code);
```
Configure and Enable the service. Parameters for configuration of the service
are provided as a dictionary / map. 

+ IN a{av} argv_dict: Each dictionary entry is a name variant pair
corresponding to the name of the property and the value respectively.
+ OUT x return_value : 0 on Success, else Failure.

An empty dictionary may be passed to re-enable a service post a disable operation.

### Disable method
```
Disable (IN as argv_list,
    OUT x return_code);
```
UnConfigure and Disable the service. Parameters for (un)configuration of the
service are provided as a list.

+ IN as argv_list: Each list item is a string corresponding to the name of the
property to be disabled.
+ OUT x return_value : 0 on Success, else Failure.
An empty list may be passed to disable ALL the parameters of the service.

### Reset method
```
Reset (OUT x return_code);
```
Reset all the configurable properties of the service to the distribution
specific default.

+ OUT x return_value : 0 on Success, else Failure.


### Start method
```
Start (OUT x return_code);
```
Start the service.

+ OUT x return_value : 0 on Success, else Failure.

### Stop method
```
Stop (OUT x return_code);
```
Stop the service.

+ OUT x return_value : 0 on Success, else Failure.


### Examples
A journal/syslog management service (org.openbmc.LogManager) is considered as
an example for the purpose of illustration. org.openbmc.Logmanager implements
the following interfaces:
```
org.freedesktop.DBus.Properties
org.openbmc.Service
org.openbmc.Errl
```
The log management service can be configured with the IP address and port
number of the remote syslog server. When Configured with the above properties
and Enabled, journal logs will be streamed as syslog entries to the remote
syslog server. When disabled, journal as well as syslog will be logged on the
locally. When Enabled again, the previous configuration will be restored and
streaming of logs will resume.

Start/Stop/Reset are not implemented by this service.

#### Configure the IP address and Port number for the remote logging service
and Enable it.
```
busctl call org.openbmc.LogManager /org/openbmc/LogManager org.openbmc.Service Enable a{sv} 2 ipaddr s 9.109.116.67 port u 514
```
#### Disable the remote logging service.

```
busctl call org.openbmc.LogManager /org/openbmc/LogManager/rsyslog org.openbmc.Service Disable as 0
```

#### Restore previous configuration and Enable the remote logging service.
```
busctl call org.openbmc.LogManager /org/openbmc/LogManager/rsyslog org.openbmc.Service Enable a{sv} 0
```

#### Get the IP address configured as the remote syslog server.
```
busctl call org.openbmc.LogManager /org/openbmc/LogManager/rsyslog org.freedesktop.DBus.Properties Get ss org.openbmc.Errl ipaddr
```

#### Get the Port number configured as the remote syslog server.
```
busctl call org.openbmc.LogManager /org/openbmc/LogManager/rsyslog org.freedesktop.DBus.Properties Get ss org.openbmc.Errl port
```

#### Get All configuration properties of the logging service.
```
busctl call org.openbmc.LogManager /org/openbmc/LogManager/rsyslog org.freedesktop.DBus.Properties GetAll s org.openbmc.Errl
```
---
Signed-off-by: Hari R. <iamrhari@gmail.com>
---
---
