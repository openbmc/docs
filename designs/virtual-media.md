# Virtual Media (a.k.a. Remote Media)

Author: Pawel Rapkiewicz <pawel.rapkiewicz@intel.com> <pawelr>

Other contributors: Przemyslaw Czarnowski
<przemyslaw.hawrylewicz.czarnowski@intel.com> Anna Platash
<anna.platash@intel.com>

Created: 6/4/2019

## Problem Description

Virtual Media allows users to remotely mount given ISO/IMG drive images through
BMC to Server HOST. The Remote drive is visible in HOST as USB storage device,
and operates in RO mode, or RW mode (keeping in mind container limitations, and
write protection switches). This can even be used to install OS on bare metal
system. This document focuses on few redirection options, like in-browser
ISO/IMG image mounting, and remote CIFS/HTTPS image mounting.

## References

- Virtual Media is going to use Network Block Device as primary disk image
  forwarder.
- NBDkit is being used, to serve images from remote storages over HTTPS/CIFS.
  USBGadget as way to expose Media Storage Device to HOST.

## Requirements

None

## Proposed Design

Virtual Media splits into two modes of operation, lets call it Proxy, and
Legacy.

- Proxy mode - works directly from browser and uses JavaScript/HTML5 to
  communicate over Secure WebSockets directly to HTTPS endpoint hosted by bmcweb
  on BMC.
- Legacy mode - is initiated from browser using Redfish defined VirtualMedia
  schemas, then BMC process connects to external CIFS/HTTPS image pointed during
  initialization.

Both methods inherit from default Redfish/BMCWeb authentication and privileges
mechanisms.

The component diagram below shows Virtual Media high-level overview

```ascii
+------------------+           +----------------------------------+     +-----------------------+
|Remote Device|    |           |BMC|              +------------+  |     |HOST|                  |
+-------------/    |           +---/ +--Dbus----->+VirtualMedia|  |     +----/                  |
|                  |           |     v            +------------+  |     |                       |
|  +------------+  |           |   +-+--------+                   |     |                       |
|  |WebBrowser  +<----HTTPS------->+BMCWeb    |      +---------+  |     |  +----------+         |
|  +------------+  |           |   +-+--------+      |USBGadget+<--------->+USB Device|         |
+------------------+           |     ^               +----+----+  |     |  +----------+         |
                               |     |                    ^       |     |                       |
                               |     |    +------+        v       |     |                       |
                               |     +--->+UNIX  |   +----+----+  |     |                       |
                               |          |SOCKET+<->+NBDClient|  |     |                       |
+------------------+           |     +--->+      |   +---------+  |     |                       |
|Remote Storage|   |           |     |    +------+                |     |                       |
+--------------/   |           |     |                            |     |                       |
|                  |           |     v                            |     |                       |
|   +-----------+  |           |   +-+-------+                    |     |                       |
|   |ISO/IMG    +<---CIFS/HTTPS+-->+NBDkit   |                    |     |                       |
|   +-----------+  |           |   +---------+                    |     +-----------------------+
|                  |           |                                  |
+------------------+           +----------------------------------+
```

Virtual Media feature supports multiple, simultaneous connections in both modes.

### Asynchronicity

Mounting and unmounting of remote device could be time consuming. Virtual Media
shall support asynchronicity at the level of DBus and optionally Redfish API.

Default timeouts for DBus (25 seconds) and for Redfish (60 seconds) may be
insufficient to perform mounting and unmounting in some cases.

Asynchronous responses will be described later on in appropriate sections.

### Network Block Device (NBD)

Reader can notice that most connections on diagram are based on Network Block
Device. After Sourceforge project description:

> With this compiled into your kernel, Linux can use a remote server as one of
> its block devices. Every time the client computer wants to read /dev/nbd0, it
> will send a request to the server via TCP, which will reply with the data
> requested. This can be used for stations with low disk space (or even
> diskless - if you use an initrd) to borrow disk space from other computers.
> Unlike NFS, it is possible to put any file system on it. But (also unlike
> NFS), if someone has mounted NBD read/write, you must assure that no one else
> will have it mounted.
>
> -- [https://nbd.sourceforge.io/](https://nbd.sourceforge.io/)

In Virtual Media use case, it's being used to pull data from remote client, and
present it into non BMC mounted `/dev/nbdXX` device. Then the block device is
being provided to Host through USB Gadget.

### USB Gadget

Part of Linux kernel that makes _emulation_ of certain USB device classes
possible through USB "On-The-Go", if connect appropriately to Host. In Virtual
Media case it emulates USB mass storage connected to HOST. The source or
redirection is block device created by nbd-client `/dev/nbdXX`

### Proxy Mode

Proxy Mode uses browser JavaScript and WebSockets support, to create JS NBD
Server. Browser is responsible for create HTTPS session, authenticate user, and
receive given privileges, then upgrade HTTPS session to WSS, through mechanisms
described by [RFC6455](HTTPS://tools.ietf.org/html/rfc6455). Since WSS upgrade,
JS application is responsible for handling all required by specification NBD
Server commands.

Multiple, simultaneous connections are supported per opening connections on
different URIs in HTTPS server. Number of available simultaneous connections is
being defined in configuration file described in next chapter.

Encryption for proxy is supported through HTTPS/WSS channel and inherits
encryption mechanisms directly from HTTPS server, all data transactions go
through bmcweb.

The initialization of connection will look as on diagram:

```ascii
┌───────┐                  ┌──────┐       ┌────────────┐          ┌─────────┐   ┌────┐ ┌─────────┐
│Browser│                  │bmcweb│       │VirtualMedia│          │NBDClient│   │uDEV│ │USBGadget│
└───┬───┘                  └──┬───┘       └─────┬──────┘          └────┬────┘   └─┬──┘ └────┬────┘
    │ establish HTTPS session │                 │                      │          │         │
    │─────────────────────────>                 │                      │          │         │
    │                         │                 │                      │          │         │
    │   upgrade to WSS on     │                 │                      │          │         │
    │    /nbd/X endpoint      │  ╔══════════════╧════╗                 │          │         │
    │─────────────────────────>  ║* bmcweb creates  ░║                 │          │         │
    │                         │  ║  /tmp/nbd.X.sock  ║                 │          │         │
    │                         │  ║* bmcweb locks new ║                 │          │         │
    │                         │  ║  connections on   ║                 │          │         │
    │                         │  ║  endpoint /nbd/X  ║                 │          │         │
    │                         │  ╚══════════════╤════╝                 │          │         │
    │                         │    Mount from:  │                      │          │         │
    │                         │ /tmp/nbd.X.sock │                      │          │         │
    │                         │ ────────────────>                      │          │         │
    │                         │                 │                      │          │         │
    │                         │                 │ Spawn NBDClient from │          │         │
    │                         │                 │    /tmp/nbd.x.sock   │          │         │
    │                         │                 │      to /dev/nbdX   ┌┴┐         │         │
    │                         │                 │ ──────────────────> │ │         │         │
    │                         │                 │                     │ │         │         │
    │                         │                 │     Block Device    │ │         │         │
    │                         │                 │  properties changed │ │         │         │
    │                         │                 │ <───────────────────────────────│         │
    │                         │                 │                     │ │         │         │
    │                         │                 │      Configure USB mass         │         │
    │                         │                 │    storage from /dev/nbd/X      │         │
    │                         │                 │ ─────────────────────────────────────────>│
    │                         │                 │                     │ │         │         │
    │                         │    Completion   │                     | |         │         │
    │                         │      signal     │                     | |         │         │
    │                         │ <───────────────│                     │ │         │         │
    │                         │                 │                     │ │         │         │
    │                         │                 │         Data        │ │         │         │
    │<─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>│
    │                         │                 │                     │ │         │         │
```

### Legacy Mode

Legacy Mode uses VirtualMedia schema, defined by DMTF, for mounting external
CIFS/HTTPS images. The current implementation supports only stream mounting as
for now. In this case Redfish is only used as mechanism for Virtual Media
initialization, and is not responsible for data transmission. For data there is
a separate component responsible for handling CIFS/HTTPS traffic called NBDkit.

Multiple, simultaneous connections are supported through spawning additional
nbkit instances, the number of available instances for CIFS/HTTPS is configured
and described in details in next chapter.

Encryption is based on remote storage connection, and follows Intel's Best
security practices, as remote server support such (i.e. HTTPS requires SSL, and
pure HTTP is not supported, for CIFS protocol version 3.0 allows enabling
encryption and that will be provided).

The flow looks like below:

```ascii
┌───────┐  ┌──────────┐  ┌──────┐  ┌────────────┐           ┌──────┐┌─────────┐┌────┐ ┌─────────┐
│Browser│  │CIFS/HTTPS│  │bmcweb│  │VirtualMedia│           │NBDkit││NBDClient││uDEV│ │USBGadget│
└───┬───┘  └────┬─────┘  └──┬───┘  └─────┬──────┘           └──┬───┘└────┬────┘└─┬──┘ └────┬────┘
    │establish HTTPS session│            │                     │         │       │         │
    │───────────────────────>            │                     │         │       │         │
    │           │           │            │                     │         │       │         │
    │Create new VirtualMedia│            │                     │         │       │         │
    │  mountpoint via POST  │            │                     │         │       │         │
    │───────────────────────>            │                     │         │       │         │
    │           │           │            │                     │         │       │         │
    │           │           │ Mount from │                     │         │       │         │
    │           │           │ CIFS/HTTPS │                     │         │       │         │
    │           │           │  location  │                     │         │       │         │
    │           │           │────────────>                     │         │       │         │
    │           │           │            │                     │         │       │         │
    │           │           │            │Spawn NBDKit mounting│         │       │         │
    │           │           │            │    given location   │         │       │         │
    │           │           │            │     appropriate     │         │       │         │
    │           │           │            │   /tmp/nbd.X.sock  ┌┴┐        │       │         │
    │           │           │            │ ──────────────────>│ │        │       │         │
    │           │           │            │                    │ │        │       │         │
    │           │           │            │      Spawn NBDClient from     │       │         │
    │           │           │            │ /tmp/nbd.X.sock to /dev/nbdX ┌┴┐      │         │
    │           │           │            │ ────────────────────────────>│ │      │         │
    │           │           │            │                    │ │       │ │      │         │
    │           │           │            │    Block Device properties changed    │         │
    │           │           │            │ <──────────────────────────────────────         │
    │           │           │            │                    │ │       │ │      │         │
    │           │           │            │    Configure USB mass storage from /dev/nbd/X   │
    │           │           │            │ ───────────────────────────────────────────────>│
    │           │           │            │                    │ │       │ │      │         │
    │           │           │ Completion │                    │ │       │ │      │         │
    │           │           │   signal   │                    │ │       │ │      │         │
    │           │           |<───────────│                    │ │       │ │      │         │
    │           │           │            │                    │ │       │ │      │         │
    │           │           │            │               Data │ │       │ │      │         │
    │           │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>│
```

### Redfish support

Virtual Media Service will be exposed as Redfish VirtualMedia endpoint as
defined by DMTF. Here are some examples.

#### Virtual Media Collection schema

Members in collection will be defined based on configuration file described in
next sections. And will be visible despite media is inserted or not.

```json
{
  "@odata.type": "#VirtualMediaCollection.VirtualMediaCollection",
  "Name": "Virtual Media Services",
  "Description": "Redfish-BMC Virtual Media Service Settings",
  "Members@odata.count": 2,
  "Members": [
    {
      "@odata.id": "/redfish/v1/Managers/BMC/VirtualMedia/ISO0"
    },
    {
      "@odata.id": "/redfish/v1/Managers/BMC/VirtualMedia/1"
    }
  ],
  "@odata.context": "/redfish/v1/$metadata#VirtualMediaCollection.VirtualMediaCollection",
  "@odata.id": "/redfish/v1/Managers/BMC/VirtualMedia"
}
```

#### Virtual Media schema

```json
{
  "@odata.type": "#VirtualMedia.v1_1_0.VirtualMedia",
  "Id": "ISO0",
  "Name": "Virtual Removable Media",
  "Actions": {
    "#VirtualMedia.InsertMedia": {
      "target": "/redfish/v1/Managers/bmc/VirtualMedia/ISO0/Actions/VirtualMedia.InsertMedia"
    },
    "#VirtualMedia.EjectMedia": {
      "target": "/redfish/v1/Managers/bmc/VirtualMedia/ISO0/Actions/VirtualMedia.EjectMedia"
    }
  },
  "MediaTypes": ["CD", "USBStick"],
  "Image": "https://192.168.0.1/Images/os.iso",
  "ImageName": "Os",
  "ConnectedVia": "URI",
  "Inserted": true,
  "WriteProtected": false,
  "@odata.context": "/redfish/v1/$metadata#VirtualMedia.VirtualMedia",
  "@odata.id": "/redfish/v1/Managers/BMC/VirtualMedia/ISO0",
  "Oem": {
    "OpenBMC": {
      "@odata.type": "#OemVirtualMedia.v1_0_0.VirtualMedia",
      "WebSocketEndpoint": "/nbd/0"
    }
  }
}
```

Schema will look similar for both Proxy and Legacy Mode. Some key differences as
follows:

| Field Name           | Proxy Mode | Legacy Mode                      | Comment                                                         |
| -------------------- | ---------- | -------------------------------- | --------------------------------------------------------------- |
| InsertMedia          | N/A        | action as described by DMTF spec | Action can return Task object if<br> process is time consumming |
| Image                | N/A        | image location                   |                                                                 |
| ImageName            | N/A        | image name                       |                                                                 |
| ConnectedVia         | "Applet"   | as described by DMTF spec        | applies only for connected media                                |
| TransferMethod       | "Stream"   | "Stream"                         | "upload" is not supported by design                             |
| TransferProtocolType | "OEM"      | as described by DMTF spec        |                                                                 |

#### Virtual Media OEM Extension

Virtual Media schema is adapted to Legacy Mode where image is given by user
directly via Redfish action and whole connection is processed between service
and web server.

For [Proxy Mode](#Proxy-Mode) nbd data is served by client web browser. Having
multiple connections, in order to setup connection, client needs the information
about the location of websocket created by web server. This value is exposed as
OEM `WebSocketEndpoint` property for each item.

### Inactivity timeout

Virtual Media supports inactivity timeout, which will break Virtual Media
connection after certain number of seconds of inactivity. Because nbdclient has
mechanism for caching image, also kernel has home buffer mechanisms for block
device, the idea is to prepare a patch on USBGadget driver, that will write USB
gadget statistics under /proc/USBGadget/lun.X file. Those statistics will be
observed by Virtual Media application.

### Virtual Media Service

Virtual Media Service is separate application that will coexist on DBus. It will
be initialized from configuration json file, and expose all available for
provisioning VirtualMedia objects. Virtual Media is responsible for:

- Exposing current Virtual Media configuration to DBus user.
- Spawning nbdclient, and monitor its lifetime, for Proxy connections.
- Spawning nbdkit, and monitor its lifetime, for CIFS/HTTPS connections.
- Monitoring uDEV for all NBD related block device changes, and
  configure/de-configure USB Gadget accordingly.
- Monitor NBD device inactivity period to support inactivity timeout.

### Configuration

Upon process startup, Virtual Media reads its config file, with the following
structure:

```json
"InactivityTimeout": 1800,              # Timeout of inactivity on device in seconds, that will lead to automatic disconnection
"MountPoints": {
    "ISO0": {
        "EndpointId": "/nbd/0",         # bmcweb endpoint (URL) configured for this type of connection
        "Mode": 0,                      # 0 - Proxy Mode, 1 - Legacy Mode
        "NBDDevice": "/dev/nbd0",       # nbd endpoint on device usually matches numeric value with EndpointId
        "UnixSocket": "/tmp/nbd.sock",  # defines which Unix socket will be occupied by connection
        "Timeout": 30,                  # timeout in seconds passed to nbdclient
        "BlockSize": 512,               # Block size passed to nbdclient
    }
},
```

### DBus Interface

Virtual Media will expose the following object structure. All object paths are
representation of configuration file described above

```ascii
/xyz/openbmc_project/VirtualMedia/Proxy/ISO0
/xyz/openbmc_project/VirtualMedia/Proxy/1
/xyz/openbmc_project/VirtualMedia/Legacy/0
/xyz/openbmc_project/VirtualMedia/Legacy/1
```

Each of object will implement `xyz.openbmc_project.VirtualMedia.Process`
interface, which will be defined as follow:

| Name     | type     | input | return  | description                                                         |
| -------- | -------- | ----- | ------- | ------------------------------------------------------------------- |
| Active   | Property | -     | BOOLEAN | `True`, if object is occupied by active process, `False` otherwise  |
| ExitCode | Property | -     | INT32   | If process terminates this property will contain returned exit code |

Each object will also expose configuration of its own under
`xyz.openbmc_project.VirtualMedia.MountPoint` (all properties are RO)

| Name                       | type     | input | return  | description                                                               |
| -------------------------- | -------- | ----- | ------- | ------------------------------------------------------------------------- |
| EndPointId                 | Property | -     | STRING  | As per configuration                                                      |
| Mode                       | Property | -     | BYTE    | As per configuration                                                      |
| Device                     | Property | -     | STRING  | As per configuration                                                      |
| Socket                     | Property | -     | STRING  | As per configuration                                                      |
| Timeout                    | Property | -     | UINT16  | As per configuration                                                      |
| BlockSize                  | Property | -     | UINT16  | As per configuration                                                      |
| RemainingInactivityTimeout | Property | -     | UINT16  | Seconds to drop connection by server, for activated endpoint, 0 otherwise |
| ImageURL                   | Property | -     | STRING  | URL to mounted image                                                      |
| WriteProtected             | Property | -     | BOOLEAN | 'True', if the image is mounted as read only, 'False' otherwise           |

Another interface exposed by each object are stats under
`xyz.openbmc_project.VirtualMedia.Stats` (all properties are RO):

| Name    | type     | input | return | description                              |
| ------- | -------- | ----- | ------ | ---------------------------------------- |
| ReadIO  | Property | -     | UINT64 | Number of read IOs since image mounting  |
| WriteIO | Property | -     | UINT64 | Number of write IOs since image mounting |

Depends on object path, object will expose different interface for mounting
image.

Mounting can be a time consuming task, so event driven mechanism has to be
introduced. Mount and Unmount calls will trigger asynchronous action and will
end immediately, giving appropriate signal containing status on task completion.

For Proxy its `xyz.openbmc_project.VirtualMedia.Proxy`, defined as follow:

| Name       | type   | input | return  | description                                                                    |
| ---------- | ------ | ----- | ------- | ------------------------------------------------------------------------------ |
| Mount      | Method | -     | BOOLEAN | Perform an asynchronous operation of mounting to HOST on given object.         |
| Unmount    | Method | -     | BOOLEAN | Perform an asynchronous operation of unmount from HOST on given object         |
| Completion | Signal | -     | INT32   | Returns 0 for success or errno on failure after background operation completes |

For Legacy its `xyz.openbmc_project.VirtualMedia.Legacy`, defined as follow:

| Name       | type   | input                                     | return  | description                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| ---------- | ------ | ----------------------------------------- | ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Mount      | Method | STRING                                    | BOOLEAN | Perform an asynchronous operation of mounting to HOST on given object, with location given as STRING parameter                                                                                                                                                                                                                                                                                                                                                                |
| Mount      | Method | STRING<br>BOOLEAN<br>VARIANT<UNIX_FD,INT> | BOOLEAN | Perform an asynchronous operation of mounting to HOST on given object, with parameters:<br><br>`STRING` : url to image. It should start with either `smb://` or `https://` prefix<br>`BOOLEAN` : RW flag for mounted gadget (should be consistent with remote image capabilities)<br>`VARIANT<UNIX_FD,INT>` : file descriptor of named pipe used for passing null-delimited secret data (username and password). When there is no data to pass `-1` should be passed as `INT` |
| Unmount    | Method | -                                         | BOOLEAN | Perform an asynchronous operation of unmounting from HOST on given object                                                                                                                                                                                                                                                                                                                                                                                                     |
| Completion | Signal | -                                         | INT32   | Returns 0 for success or errno on failure after background operation completes                                                                                                                                                                                                                                                                                                                                                                                                |

Mount and unmount operation return true if async operation is started and false
when preliminary check encountered an error. They may also indicate appropriate
dbus error.

## Alternatives Considered

Existing implementation in OpenBMC

## Impact

Shall not affect usability of current Virtual Media implementation

## Testing

TBD
