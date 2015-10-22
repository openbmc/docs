
# Host IPMI

## `/org/openbmc/HostIpmi`

signals:

   *  `ReceivedMessage(seq : byte, netfn : byte, lun : byte, cmd : byte, data: array[byte])`

methods:

   *  `sendMessage(seq : byte, netfn : byte, lun : byte, cmd : byte, cc : byte, data : array[byte])`
   
  * method: `setAttention()`

 
