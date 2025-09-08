# OpenBMC Remote KVM Access Guide

This guide explains how to access a remote KVM (Keyboard, Video, Mouse) session
using the OpenBMC management system.

The remote KVM feature allows you to view the host system's display and interact
with it using your local keyboard and mouse. All input is securely transmitted
to the host via the BMC.

## Accessing Remote KVM via Web Interface

1. Log in to the BMC Web UI.
2. Navigate to **Operations** > **KVM**.
3. The host's graphical output will be displayed, and you can interact with it
   directly from your browser.

## Accessing Remote KVM via VNC Client

Direct access to bmc-kvm-port (usually 5900) on the BMC is typically blocked
for security reasons. To securely connect to the KVM, use a VNC client over an
SSH tunnel.

**Note:** SSH local port forwarding must be enabled on the BMC.  
If you are using Dropbear, make sure it is started without the `-j` option.

1. Create an SSH tunnel to forward the KVM port from the BMC to your local
   machine:

   ```bash
   ssh -L <localport>:localhost:<bmc-kvm-port> <bmc-user>@<bmc-hostname>
   ```

   - `localport`: The port number your VNC client will use locally (e.g., 1234)
   - `bmc-kvm-port`: The port used by the KVM service on the BMC (usually 5900)
   - `bmc-user`: Your SSH username for the BMC
   - `bmc-hostname`: The IP address or hostname of the BMC

   **Example:**

   ```bash
   ssh -L 1234:localhost:5900 root@192.168.1.1
   ```

2. Open your VNC client and connect to `localhost:<localport>`. For example,
   `localhost:1234`.

You should now be able to view and interact with the host's graphical output as
if you were using a local monitor, keyboard, and mouse.
