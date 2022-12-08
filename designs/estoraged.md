# eStoraged Design - Encrypted Secondary Storage Management Daemon

Author: John Wedig (johnwedig@google.com)

Other contributors: John Broadbent (jebr@google.com) Benjamin Fair
(benjaminfair@google.com) Nancy Yuenn (yuenn@google.com)

Created: September 2, 2021

## Problem Description

This daemon will serve as an abstraction for an encrypted storage device,
encapsulating the security functionality and providing a D-Bus interface to
manage the encrypted filesystem on the device. Using the D-Bus interface, other
software components can interact with eStoraged to do things like create a new
encrypted filesystem, wipe its contents, lock/unlock the device, or change the
password.

## Background and References

This design is intended to manage secondary storage devices and cannot be used
for the root filesystem, i.e. the BMC needs to be able to boot while the device
is still locked.

This design makes use of the
[cryptsetup](https://gitlab.com/cryptsetup/cryptsetup) utility, which in turn
uses the [dm-crypt](https://en.wikipedia.org/wiki/Dm-crypt) kernel subsystem.
Dm-crypt provides the encryption and device mapping capability, and Cryptsetup
provides the [LUKS](https://en.wikipedia.org/wiki/Linux_Unified_Key_Setup)
capability, which facilitates password management so that we can do things like
change the password without re-encrypting the entire device.

This design is specifically targeted for use with eMMC devices, and we plan to
make use of the lock/unlock feature (CMD42) at the eMMC hardware level as an
additional security measure. This feature prohibits all read or write accesses
to the device while locked. Some documentation on this feature can be found in
the
[JEDEC standard JESD84-B51A](https://www.jedec.org/document_search?search_api_views_fulltext=jesd84-b51),
or in this document:
[Enabling SD/uSD Card Lock/Unlock Feature in Linux](https://media-www.micron.com/-/media/client/global/documents/products/technical-note/sd-cards/tnsd01_enable_sd_lock_unlock_in_linux.pdf?rev=03f03a6bc0f8435fafa93a8fc8e88988).

There are several types of keys referenced in this doc:

- Volume key: The main encryption key used to encrypt the data on the block
  device.
- Encryption Password: The password needed to load the volume key into RAM and
  decrypt the filesystem.
- Device Password: The password to lock or unlock the device hardware.

## Requirements

This design should provide an interface for the following capabilities:

- Create a new LUKS encrypted filesystem on the device
- Securely wipe the device and verify that the data was wiped
- Lock the device
- Unlock the device
- Change the password

In addition, eStoraged should:

- Generate a volume key using a random number generator with enough entropy,
  making it sufficiently random and secure.
- Utilize any security features provided by the hardware (as a defense-in-depth
  measure).
- Use interfaces that are generic enough, so that they can be extended to
  support additional types of storage devices, as desired. For example,
  different devices will have different command sets for device locking, e.g.
  MMC, SATA, NVMe. Initially, we plan to only use eStoraged with eMMC devices,
  but we may wish to use this with other types of storage devices in the future.

The users of this design can be any other software component in the BMC. Some
client daemon on the BMC will interact with eStoraged to set up a new encrypted
filesystem on the eMMC. In addition, the client daemon could be configured to
unlock the eMMC device when the BMC boots. It is the responsibility of the
client daemon to provide a password. For example, this password could come from
user input, fetched from a secure location, or the client daemon could generate
the passwords itself.

## Proposed Design

eStoraged will represent each eMMC device as an object on D-Bus that implements
an interface providing these methods and properties:

- (method) Format
- (method) Erase
- (method) Lock
- (method) Unlock
- (method) Change Password
- (property) Locked
- (property) Status

Upon startup, eStoraged will create a D-Bus object for each eMMC device in the
system. Specifically, we will use udev to launch an eStoraged instance for each
eMMC. The bus name and object name will be as follows:

Bus Name: xyz.openbmc_project.eStorage.\<device name\> Object Path:
/xyz/openbmc_project/storage/\<device name\>

The object path is intended to be generic enough, so that we could ultimately
have multiple daemons managing the same storage device, while using the same
object path. For example, this daemon would handle the encryption, whereas
another daemon could provide stats for the same device.

To manage the encrypted filesystem, we will make use of the
[cryptsetup API](https://mbroz.fedorapeople.org/libcryptsetup_API/). This
library provides functions to create a new LUKS header, set the password, etc.

For eMMC devices, we plan to enable the password locking feature (CMD42), to
prevent all read or write accesses to the device while locked. So, the "Locked"
property will mean both locked at the hardware level and locked at the
encryption level. We will likely use the ioctl interface to send the relevant
commands to the eMMC, similar to what
[mmc utils](https://git.kernel.org/pub/scm/utils/mmc/mmc-utils.git/) does.

Support for hardware locking on other types of devices can be added as needed,
but at the very least, encryption-only locking will be available, even if
hardware locking isn't supported for a particular device.

As mentioned earlier, the client will provide a password. This password will be
used by eStoraged to generate two different passwords: the encryption password
and the device password (if hardware locking is available). The passwords will
be different, so that in the event that one password is compromised, we still
have some protection from the other password.

The Erase method should provide a way to specify the type of erase, e.g. write
all zeros, or do something else. Different organizations may have different
opinions of what a secure erase entails.

Since some of the D-Bus methods may take a while (e.g. installing a new
encrypted filesystem), the D-Bus interface will be asynchronous, with the
"Status" property that can be queried to indicate one of the following: success,
error, or in-progress.

## Alternatives Considered

An alternative would be to use systemd targets to manage the eMMC. For example,
the
[systemd-cryptsetup@.service](https://www.freedesktop.org/software/systemd/man/systemd-cryptsetup@.service.html)
is often used to unlock an encrypted block device, where it takes the password
from a key file or from user input. However, the OpenBMC architecture calls for
using D-Bus as the primary form of inter-process communication. In addition,
using a daemon with a well-defined D-Bus interface keeps the security
functionality more isolated, maintainable, and testable.

Another related piece of software is UDisks2, which also exports a D-Bus object
for each storage device in a system. It is capable of setting up an encrypted
block device with the Format method:
[org.freedesktop.UDisks2.Format](http://storaged.org/doc/udisks2-api/latest/gdbus-org.freedesktop.UDisks2.Block.html#gdbus-method-org-freedesktop-UDisks2-Block.Format).
And it provides several additional methods related to encryption: Lock, Unlock,
and ChangePassphrase. See the D-Bus interface
[org.freedesktop.UDisks2.Encrypted](http://storaged.org/doc/udisks2-api/2.7.5/gdbus-org.freedesktop.UDisks2.Encrypted.html).
The main problem preventing us from leveraging this tool is that it increases
our image size too much. We found that the compressed image size increased by 22
MB due to the transitive dependencies being pulled in, e.g. mozjs and python.

## Impacts

To make use of eStoraged, it may be necessary to provide another client daemon
that manages the password and invokes the D-Bus API for eStoraged. Since the
password management scheme can be unique for different platforms and
organizations, it is outside the scope of this design.

## Testing

- Unit tests to validate the various code paths in eStoraged.
- Regression tests will exercise the various D-Bus methods: encrypt, erase,
  lock, unlock, and change password.
