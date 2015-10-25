
# Host IPMI

## `/org/openbmc/HostIpmi`

signals:

  *  `ReceivedMessage(seq : byte, netfn : byte, lun : byte, cmd : byte, data: array[byte])`

methods:

  *  `sendMessage(seq : byte, netfn : byte, lun : byte, cmd : byte, cc : byte, data : array[byte])`
   
  *  `setAttention()`

 
# Useful dbus cli tools

## 'busctl'  -  http://www.freedesktop.org/software/systemd/man/busctl.html
Great tool to issue rest commands via cli.  That way you don't have to wait for the code to hit the path on the system.  Great for running commands with QEMU too!

busctl call \<path\> \<interface\> \<object\> \<method\> \<parameters\> 

* \<parameters\> example : sssay "t1" "t2" "t3" 2 2 3
