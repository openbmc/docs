# Firewall Configuration Support

Author: Rashid MP <rashidmp@ami.com>

Other contributors: None

Created: April 6, 2026

## Problem Description

BMC systems are increasingly deployed in security-sensitive environments where
network-level access control is critical. Current OpenBMC provides
service-config-manager for enabling/disabling entire services (SSH, IPMI,etc.)
and ManagerNetworkProtocol for protocol-level controls, but lacksthe ability to
restrict access based on source network or IP address.

This creates a significant security gap: services can only be enabled for
everyone or disabled for everyone. There is no way to implement policieslike
"allow SSH only from management network" or "restrict Redfish API to specific
jump hosts." In multi-tenant data centers, this forces operators to either
expose BMC management to all networks (security risk) or disable services
entirely (operational limitation).

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

The solution must provide D-Bus methods to add, delete, flush, and query
firewall rules. These D-Bus interfaces will be consumed by bmcweb to expose
Redfish API endpoints, allowing administrators to manage firewall rules through
WebUI or Redfish clients (curl, redfishtool, etc.).

Rule matching must support:

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
evaluated in order of insertion.

**Example use cases:**

- Restrict Redfish API access to management network 10.0.0.0/8
- Block SSH except from specific jump host IPs
- Allow IPMI only during scheduled maintenance time windows
- Query existing rules before making configuration changes
- Remove temporary maintenance window access rules
- Flush all rules and rebuild firewall policy from scratch

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

The D-Bus interface provides four methods: AddRule, DelRule, FlushAll, and
GetRules. Rules are represented as tuples containing target (ACCEPT/DROP),
control bitmask, protocol, IP range, port range, MAC address, and time range.
The control bitmask indicates which fields are active in the rule.

Rules are executed using iptables/ip6tables command-line tools via execvp() with
proper argument escaping. After each modification, rules are persisted using
iptables-save to /etc/iptables/rules and /etc/iptables/ip6tables_rules. On
service startup, iptables-restore loads saved rules.

The build system uses meson options and Yocto DISTRO_FEATURES to control
compilation. When enabled, iptables package becomes a runtime dependency and
kernel configuration fragments enable required netfilter modules.

## Alternatives Considered

**ufw (Uncomplicated Firewall)**: ufw is a user-friendlyfrontend for iptables
written in Python, commonly used in Ubuntu systems. However,ufw is designed for
CLI-based configuration using configuration files in /etc/ufw/ and does not
provide a D-Bus API for programmatic access. Integrating ufw with bmcweb's
Redfish API would require shell script wrappers, which introduces security
concerns and doesn't follow OpenBMC architectural patterns. Additionally, ufw
adds Python dependencies to the BMC image.

**firewalld**: firewalld provides a complete firewall management framework and
D-Bus API for managing firewall policies. However, the requirements of this
proposal are limited to firewall rule management and access control. Adopting
firewalld would introduce an additional firewall management framework that
OpenBMC would need to integrate and maintain. A lightweight implementation
integrated with phosphor-networkd provides the required functionality with a
simpler architecture.

**nftables**: nftables is the modern successor to iptables and provides the
packet-filtering capabilities required by this proposal. Both nftables and
iptables are suitable implementation choices for the proposed firewall
interface. The initial implementation uses iptables because it satisfies the
current requirements while keeping the design focused on introducing firewall
management support within OpenBMC. The proposed functionality does not require
features unique to nftables.

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
add/delete/flush/get operations for IPv4 and IPv6 rules, including complex
scenarios like IP ranges, port filtering, MAC filtering, and time-based rules.
Persistence across service restart will be tested.

Build testing will verify the feature compiles correctly when enabled and
disabled. CI testing will include builds with firewall-support enabled for
selected platforms. The feature has been manually tested on evb-ast2600 with
successful rule management and persistence verification.
