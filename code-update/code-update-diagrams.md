# Code Update Diagrams

1. [High-Level Overview](#High-Level Overview)

## High-Level Overview

```
  ┌─────────┐              ┌─────────────┐       ┌────────────┐
  │  User   │              │Image Manager│       │Item Updater│
  └───┬─────┘              └──────┬──────┘       └──────┬─────┘
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
      │                           │   Create            │
      │                           │  Software           │
      │                           │D-Bus object         │
      │                           │      │              │
      │                           ├──────┘              │
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
      │                                                 │   flash
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
