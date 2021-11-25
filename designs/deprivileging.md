# Privilege separation

Author:
  rnouse

Primary assignee:
  rnouse

Created:

Issue:
  https://github.com/openbmc/openbmc/issues/3383

## Problem Description

At the moment, daemons in OpenBMC run as root and communicate to each other via D-Bus without any restrictions. The purpose of this work is to make daemons run as regular users (reduced privileges) and introduce ACLs for D-Bus communication.

# Proposed design
systemd has a **DynamicUser** service unit option to allocate, handle and recycle dynamic users/groups. 

## Service Unit
The regular Service file looks like:
```
[Unit]
Description=Network IPMI daemon
Requires=phosphor-ipmi-host.service
After=phosphor-ipmi-host.service

[Service]
ExecStart=/usr/bin/netipmid -c %i
SyslogIdentifier=netipmid-%i
Restart=always
RuntimeDirectory = ipmi
RuntimeDirectoryPreserve = yes
StateDirectory = ipmi

PrivateTmp = yes
DynamicUser = yes
Group = ipmi
SupplementaryGroups = ipmi

UMask = 0007
AmbientCapabilities = CAP_NET_BIND_SERVICE

[Install]
DefaultInstance=eth0
WantedBy=multi-user.target
RequiredBy=
```

Each service file is supplied with the corresponding service's source, thus, is easy to maintain.

The strictness of the applied security rules should be balanced with the further effort of support and security benefits.

## D-Bus ACL
D-Bus broker supports `busconfig` configurations that allow explicitly controlling the callee & callers down to the specific method/property. Here is an [example](https://www.apt-browse.org/browse/debian/wheezy/main/amd64/systemd/44-11+deb7u4/file/etc/dbus-1/system.d/org.freedesktop.systemd1.conf).

Thus, the ACL rules might looks like:
```xml
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="dbus-mapper">
    <allow own="xyz.openbmc_project.ObjectMapper"/>
    <allow send_destination="xyz.openbmc_project.ObjectMapper"/>
  </policy>
  <policy group="ipmi">
    <allow send_destination="xyz.openbmc_project.ObjectMapper"/>
    <allow receive_sender="xyz.openbmc_project.ObjectMapper"/> 
    <allow send_interface="xyz.openbmc_project.ObjectMapper"/>
  </policy>
</busconfig>
```

## Notes
The `busconfig` ACLs checks before sd-bus being involved to [check access capability](https://github.com/systemd/systemd/blob/935052a8aa11329061cbee234c99b03973163594/src/libsystemd/sd-bus/bus-objects.c#L298). Thus, we can control everything on the D-Bus level (via dbus-broker) to ACL the calls, property access, but have to expose methods in **vtable** as unprivileged.

Current per-service [tracking](https://github.com/openbmc/openbmc/wiki/Security---Privilege-separation-&-Sandboxing) of D-Bus methods/interfaces and filesystem access.

## Namespaces & seccomp
With follow-up work it might be considered to use network/filesystem namespaces isolation as well as using seccomp filters for deeper process hardening.

## Why enforcing busconfig ACLs first
There was a discussion in Discord to start moving daemons to an unprivileged space ahead of enforcing busconfig ACLs. As per the notes above, all daemons that are built on top of the systemd-dbus implementation checks the callee’s capabilities regardless of busconfig ACLs. Therefore, sdbusplus should be modified to expose methods as unprivileged in order to handle ACLs by dbus-broker daemon.

## Open Questions
Special cases for PAM-dependent services (netipmid, bmcweb) will require a separate privileged process that handles requests to PAM. Possible approaches need to be discussed.

## Impact
This work will prevent possibility of root access and gives less power for an attacker in case if daemons are being compromised.

## Testing
During the development changes can be tested manually within QEMU run on the various supported platforms as well as CI testing.

# Roadmap
 * Modify sdbusplus to expose methods as unprivileged.
 * Implement & enforce busconfig ACLs for processes running as root. This has to be done per service/subproject.
 * Move services to run as unprivileged users
   * Use migration postinsts scripts to chown/chgrp and move files around.
   * Special cases for PAM-dependent services (netipmid, bmcweb) will require a privilege process that handles PAM. The alternative approach was to use a **sssd** daemon, but it has a lot of hard dependencies that significantly increases the image size (+25M to unpacked rootfs).
