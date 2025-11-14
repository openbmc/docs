### Use `qemu-aspeed` Instead of `qemu-system-arm` for Reliable WebUI Access Across Network Setups
This change updates the documentation to recommend using the `qemu-aspeed` wrapper script instead of invoking `qemu-system-arm` directly. The wrapper provides a consistent and reliable network configuration, resolving WebUI accessibility issues that occur in mixed-network environments (e.g., WSL vs. build machine).

## Background / Problem
When using `qemu-system-arm` to run the OpenBMC image for the AST2600 EVB, the WebUI could not be accessed from a Windows host system. The issue occurred because:
- The build machine and the AST2600 EVB were on the same network.
- The WSL environment used a different virtual network.
- QEMU’s default user-mode networking caused port forwarding (e.g., 2443, 2222) to fail from the external network.
- This required accessing the WebUI using the build machine’s IP address as a workaround.

## Why `qemu-aspeed`?
The `qemu-aspeed` wrapper script:
- Automatically configures correct networking and port forwarding  
 (WebUI → 2443, SSH → 2222)
- Ensures consistent behavior across:
 - WSL
 - Native Linux environments
 - Build machines
- Simplifies invocation and reduces user configuration errors
Switching to `qemu-aspeed` resolves the WebUI accessibility problems across these setups.

## Verification
The updated instructions were tested using the EVB-AST2600 image:
-System boots successfully using `qemu-aspeed`
-WebUI accessible at:  
 `https://<build-machine-ip>:2443`
-SSH access works:  
 `ssh root@localhost -p 2222`
-Verified on:
 - WSL2
 - Native Linux
 - Build machine environment

## Summary
Using `qemu-aspeed` ensures stable WebUI access and consistent network behavior across different host setups.
