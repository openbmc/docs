# Firewall Configuration Support

Author: Rashid MP <rashidmp@ami.com>

Other contributors: None

Created: April 6, 2026

## Problem Description

BMC systems are increasingly deployed in security-sensitive environments where
network access control is critical. Currently, OpenBMC lacks a standardized
mechanism for configuring firewall rules, forcing administrators to manually
configure iptables through shell access or maintain custom scripts. This
approach is error-prone, difficult to audit, and inconsistent across
deployments. Without proper firewall controls, BMCs expose all services (HTTPS,
SSH, IPMI, Redfish) to any network-accessible host, creating significant
security risks in multi-tenant data centers and edge deployments.

A D-Bus interface for firewall configuration enables centralized management
through Redfish or custom tools, allowing administrators to restrict BMC service
access by IP address, port, protocol, MAC address, and time windows. This
addresses real security requirements such as limiting BMC access to specific
management networks, blocking unused protocols, and implementing time-based
access controls for maintenance windows.

## Background and References

OpenBMC's phosphor-networkd manages network configuration through D-Bus
interfaces at xyz.openbmc_project.Network, providing methods for configuring IP
addresses, VLANs, DNS, and routing. This design extends that model to include
firewall management, maintaining architectural consistency.

Linux netfilter is the kernel framework for packet filtering and NAT, with
iptables and ip6tables as the standard userspace configuration tools. Despite
being superseded by nftables in newer kernels, iptables remains widely deployed
in embedded systems due to its maturity, smaller footprint, and extensive
tooling support. The iptables-save/iptables-restore utilities provide efficient
rule serialization for persistence.

Current OpenBMC deployments handle firewall configuration inconsistently: some
platforms include vendor-specific scripts, others rely on manual systemd service
units, and many have no firewall protection at all. This fragmentation makes
security audits difficult and prevents consistent policy enforcement across
different BMC implementations.

References:

- [iptables man page](https://linux.die.net/man/8/iptables) - Linux packet
  filtering administration
- [Netfilter project](https://www.netfilter.org/) - Linux kernel firewall
  framework
- [phosphor-networkd](https://github.com/openbmc/phosphor-networkd) - OpenBMC
  network manager
- [phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces) -
  D-Bus interface definitions

## Requirements

### Functional Requirements

The solution must provide D-Bus methods to add, delete, flush, query, and
reorder firewall rules. Rule matching must support:

1. **IP-based filtering**: Single IP addresses (192.168.1.10) or IP ranges
   (192.168.1.10-192.168.1.20) for both IPv4 and IPv6
2. **Protocol filtering**: TCP, UDP, ICMP/ICMPv6, or all protocols
3. **Port filtering**: Single ports (443) or port ranges (8000-9000) for TCP/UDP
   protocols
4. **MAC address filtering**: Link-layer source address matching
5. **Time-based filtering**: Rules active only during specified time windows for
   maintenance schedules

Each rule must specify an action (ACCEPT or DROP) and support combining multiple
match criteria (e.g., allow TCP port 443 from 192.168.1.0/24). Rules are
evaluated in order, so reordering capability is required for priority
management.

Use cases include:

- Restricting Redfish API access to specific management networks
- Blocking SSH except from jump hosts
- Allowing IPMI only during scheduled maintenance windows
- Rate-limiting ICMP to prevent DoS reconnaissance

### Non-Functional Requirements

The feature must be compile-time optional via DISTRO_FEATURES flag, adding zero
overhead when disabled. Rule operations must complete within 100ms to maintain
responsive management interfaces. Rules must persist across BMC reboots and
phosphor-networkd restarts, with automatic restoration on service startup.

The implementation must validate all inputs (IP addresses, port numbers, time
formats) and reject invalid configurations with descriptive error codes. Rule
limits must prevent memory exhaustion. The D-Bus interface must follow OpenBMC
naming conventions (xyz.openbmc_project.Network namespace) and be generated from
YAML specifications for consistency with other phosphor interfaces.

## Proposed Design

A new FirewallConfiguration D-Bus object will be added to phosphor-networkd at
/xyz/openbmc_project/network/firewall, instantiated only when the
firewall-support DISTRO_FEATURE is enabled. The implementation uses conditional
compilation (#ifdef FIREWALL_SUPPORT) to ensure zero overhead when disabled.

```text
+------------+          +-----------+          +------------+
| WebUI      |          | Redfish   |          | D-Bus      |
| (Browser)  |          | Client    |          | Tools      |
+-----+------+          +-----+-----+          +-----+------+
      |                       |                      |
      | HTTPS                 | HTTPS                |
      v                       v                      |
+---------------------------+                        |
|        bmcweb             |                        |
| (Redfish API Server)      |                        |
+-------------+-------------+                        |
              | D-Bus API                            |
              +-------------------+------------------+
                                  |
                                  v
              +-------------------+-----------------+
              |       phosphor-networkd             |
              |  +-------------------------------+  |
              |  | FirewallConfiguration         |  |
              |  | - AddRule()                   |  |
              |  | - DelRule()                   |  |
              |  | - FlushAll()                  |  |
              |  | - GetRules()                  |  |
              |  | - ReorderRules()              |  |
              |  +---------------+---------------+  |
              |                  |                  |
              |  +---------------v---------------+  |
              |  | Persistence (/etc/iptables/*) |  |
              |  +-------------------------------+  |
              +-------------------+-----------------+
                                  |
                                  v
              +-------------------+-----------------+
              |   iptables / ip6tables              |
              |   Linux Kernel Netfilter            |
              +-------------------------------------+
```

The D-Bus interface provides five methods: AddRule, DelRule, FlushAll, GetRules,
and ReorderRules. Rules are represented as tuples containing target
(ACCEPT/DROP), control bitmask, protocol, IP range, port range, MAC address, and
time range. The control bitmask indicates which fields are active in the rule.

Rules are executed using iptables/ip6tables command-line tools via execvp() with
proper argument escaping. After each modification, rules are persisted using
iptables-save to /etc/iptables/rules and /etc/iptables/ip6tables_rules. On
service startup, iptables-restore loads saved rules.

The build system uses meson options and Yocto DISTRO_FEATURES to control
compilation. When enabled, iptables package becomes a runtime dependency and
kernel configuration fragments enable required netfilter modules.

## Impacts

**API Impact**: New D-Bus interface xyz.openbmc_project.Network.
FirewallConfiguration added. No changes to existing APIs.

**Security Impact**: Improves security by providing centralized firewall
management. D-Bus method calls require appropriate privileges. Input validation
prevents injection attacks and resource exhaustion.

**Performance Impact**: No impact when feature is disabled (compile-time
conditional). When enabled, rule operations are fast (command execution overhead
only). No impact on existing network operations or data path.

**Developer Impact**: Developers can optionally enable firewall-support distro
feature. No impact on existing code paths when disabled.

**Upgradability Impact**: Fully backward compatible. Optional feature with no
database migrations. Rules persist in standard iptables-save format.

### Organizational

- Does this proposal require a new repository? **No**
- Who will be the initial maintainer(s) of this repository? **N/A** - changes
  are in existing repositories
- Which repositories are expected to be modified to execute this design?
  - **phosphor-dbus-interfaces** (add FirewallConfiguration YAML)
  - **phosphor-networkd** (implement firewall configuration)
  - **bmcweb** (Redfish API for firewall management)
  - **openbmc** (meta-layer changes for build integration)
- Repository maintainers have been added to the Gerrit review:
  - Ratan Gupta, William Kennington III (phosphor-networkd)
  - Patrick Williams (phosphor-dbus-interfaces)
  - Ed Tanous (bmcweb)
  - Patrick Williams, Andrew Geissler (openbmc)

## Testing

Unit tests will validate rule parameter validation, iptables command generation,
error handling, and persistence logic. Integration tests will verify
add/delete/flush/get/reorder operations for IPv4 and IPv6 rules, including
complex scenarios like IP ranges, port filtering, MAC filtering, and time-based
rules. Persistence across service restart will be tested. Reorder operations
will verify atomic rollback on failure.

Build testing will verify the feature compiles correctly when enabled and
disabled. CI testing will include builds with firewall-support enabled for
selected platforms. The feature has been manually tested on evb-ast2600 with
successful rule management and persistence verification.
