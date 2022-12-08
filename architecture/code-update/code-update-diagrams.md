# Code Update Diagrams

1. [High-Level Overview](#High-Level Overview)

## High-Level Overview

```
┌──────────────┐            ┌─────────────┐       ┌────────────┐
│User Interface│            │Image Manager│       │Item Updater│
└──────┬───────┘            └──────┬──────┘       └──────┬─────┘
       │          Upload           │                     │
       │         Firmware          │                     │
       │       Image to BMC        │                     │
       ├──────────────────────────▶│                     │
       │                           │                     │
       │                           │  Extract            │
       │                           │   image             │
       │                           │ contents            │
       │                           │     │               │
       │                           ├─────┘               │
       │                           ▼                     │
       │                           │                     │
       │                           │    Create           │
       │                           │Software D-Bus       │
       │                           │  object[1]          │
       │                           │       │             │
       │                           ├───────┘             │
       │                           ▼                     │
       │                           │                     │
       │         Request to        ●                     │
       │          Activate                               │
       │          Software                               │
       │        D-Bus Object                             │
       ├────────────────────────────────────────────────▶│
       │                                                 │   Verify
       │                                                 │  digital
       │                                                 │ signatures
       │                                                 │      │
       │                                                 ├──────┘
       │                                                 ▼
       │                                                 │
       │                                                 │   Write
       │                                                 │ image to
       │                                                 │  flash[*]
       │                                                 │     │
       │                                                 ├─────┘
       │                                                 ▼
       │                                                 │
       │                                     Success     │
       │◀────────────────────────────────────────────────┤
       │                                                 │
       │                                                 ●
       ▼
   BMC Reboot is
 required to boot
 from the updated
       image

```

- [1]
  [Software D-Bus Object](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Software)
- [*] In a static layout configuration, the images are stored in RAM and the
  content is written to flash during BMC reboot. Reference the update and
  shutdown scripts provided by
  [initrdscripts](https://github.com/openbmc/openbmc/tree/master/meta-phosphor/recipes-phosphor/initrdscripts)
