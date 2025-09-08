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

To connect using a VNC client, follow these steps:

1. Establish an SSH tunnel to the BMC to securely forward the KVM port:

   ```bash
   ssh -L 1234:localhost:5900 [user]@[bmc-hostname]
   ```

2. Open your preferred VNC client and connect to `localhost:1234`.

You should now see the host's graphical output and be able to interact with it
as if you were using a local monitor, keyboard, and mouse.
