
# eStoraged Design - Encrypted eMMC Management Daemon

Author: John Wedig (johnwedig@google.com)

Primary assignee: John Wedig

Other contributors:
Benjamin Fair (benjaminfair@google.com)
Nancy Yuenn (yuenn@google.com)

Created: September 2, 2021

## Problem Description

This daemon will serve as an abstraction for an encrypted eMMC storage device,
encapsulating the security functionality and providing a D-Bus interface to
manage the encrypted filesystem on the device. Using the D-Bus interface, other
software components can interact with eStoraged to do things like create a
new encrypted filesystem, wipe its contents, lock/unlock the device, or change
the password.

## Background and References

This design makes use of the
[cryptsetup](https://gitlab.com/cryptsetup/cryptsetup) utility, which in turn
uses the [dm-crypt](https://en.wikipedia.org/wiki/Dm-crypt) kernel subsystem.
Dm-crypt provides the encryption and device mapping capability, and Cryptsetup
provides the [LUKS](https://en.wikipedia.org/wiki/Linux_Unified_Key_Setup)
capability, which facilitates password management so that we can do things like
change the password without re-encrypting the entire device.

In addition to encrypting the contents on the eMMC, this design also utilizes
the lock/unlock feature (CMD42) at the eMMC hardware level. This feature
prohibits all read or write accesses to the device while locked. Some
documentation on this feature can be found in the
[JEDEC standard JESD84-B51A](https://www.jedec.org/document_search?search_api_views_fulltext=jesd84-b51), or in this document:
[Enabling SD/uSD Card Lock/Unlock Feature in Linux](https://media-www.micron.com/-/media/client/global/documents/products/technical-note/sd-cards/tnsd01_enable_sd_lock_unlock_in_linux.pdf?rev=03f03a6bc0f8435fafa93a8fc8e88988).

There are several types of keys referenced in this doc:

- Volume key: The main encryption key used to encrypt the data on the eMMC.
- Encryption Password: The password needed to load the volume key into RAM and
  decrypt the filesystem.
- Device Password: The password to lock or unlock the eMMC hardware.

## Requirements

This design should provide an interface for the following capabilities:
- Create a new LUKS encrypted filesystem on the eMMC
- Securely wipe the device and verify that the data was wiped
- Lock the eMMC
- Unlock the eMMC
- Change the password

In addition, eStoraged should:
- Utilize any security features provided by the eMMC itself (as a
  defense-in-depth measure).
- Generate a volume key using a random number generator with enough entropy,
  making it sufficiently random and secure.

The users of this design can be any other software component in the BMC. To
give an example of how eStoraged might be used, the host CPU may issue a
request over IPMI to install a new encrypted filesystem on the eMMC, and then
that IPMI daemon will, in turn, interact with eStoraged to fulfill the request.
Alternatively, a request could arrive over the network, where a network daemon
would need to interact with eStoraged. The client(s) that interact with
eStoraged will be responsible for providing the necessary passwords (encryption
password and device password).

## Proposed Design

eStoraged will represent each eMMC device as an object on D-Bus that implements
an interface providing these methods and properties:
- (method) Encrypt
- (method) Erase
- (method) Lock
- (method) Unlock
- (method) Change Password
- (property) Locked
- (property) Status

Upon statup, eStoraged will query the system for any eMMC devices (e.g. through
sysfs or systemd) and create a D-Bus object for each one. The bus name and
object name will be as follows:

Bus Name: xyz.openbmc_project.eStorage
Object Path: /xyz/openbmc_project/encrypted_storage

If there are multiple eMMC devices, we can add a number to the end of the
object path.

To manage the encrypted filesystem, we will make use of the
[cryptsetup API](https://mbroz.fedorapeople.org/libcryptsetup_API/). This
library provides functions to create a new LUKS header, set the password, etc.

To further protect the contents of the eMMC, the password locking feature
(CMD42) will be enabled, to prevent all read or write accesses to the device
while locked. So, the "Locked" property means both locked at the hardware level
and locked at the encryption level.

Since some of the D-Bus methods may take a while (e.g. installing a new
encrypted filesystem), the D-Bus interface will be asynchronous, with the
"Status" property that can be queried to indicate one of the following:
success, error, or in-progress.

## Alternatives Considered
An alternative would be to use systemd targets to manage the eMMC. For example,
the
[systemd-cryptsetup@.service](https://www.freedesktop.org/software/systemd/man/systemd-cryptsetup@.service.html)
is often used to unlock an encrypted block device, where it takes the password
from a key file or from user input. However, the OpenBMC architecture calls for
using D-Bus as the primary form of inter-process communication. In addition,
using a daemon with a well-defined D-Bus interface keeps the security
functionality more isolated, maintainable, and testable.

## Impacts
None

## Testing
- Unit tests to validate the various code paths in eStoraged.
- Regression tests will exercise the various D-Bus methods: encrypt, erase,
  lock, unlock, and change password.
