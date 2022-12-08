# OpenBMC host console support

This document describes how to connect to the host UART console from an OpenBMC
management system.

The console infrastructure allows multiple shared connections to a single host
UART. UART data from the host is output to all connections, and input from any
connection is sent to the host.

## Remote console connections

To connect to an OpenBMC console session remotely, just ssh to your BMC on
port 2200. Use the same login credentials you would for a normal ssh session:

    $ ssh -p 2200 [user]@[bmc-hostname]

## Local console connections

If you're already logged into an OpenBMC machine, you can start a console
session directly, using:

    $ obmc-console-client

To exit from a console, type:

    return ~ .

Note that if you're on an ssh connection, you'll need to 'escape' the ~
character, by entering it twice:

    return ~ ~ .

This is because obmc-console-client is an ssh session, and a double `~` is
required to escape the "inner" (obmc-console-client) ssh session.

## Logging

Console logs are kept in:

    /var/log/obmc-console.log

This log is limited in size, and will wrap after hitting that limit (currently
set at 16kB).
