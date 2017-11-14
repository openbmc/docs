## Event subscription protocol
It is possible to subscribe to events, of interest, occurring on the BMC. The
implementation on the BMC uses WebSockets for this purpose, so that clients
don't have do employ polling. Instead, the rest server on the BMC can push
data to clients over a websocket. The BMC can push out information
pertaining to D-Bus InterfacesAdded and PropertiesChanged signals.

Following is a description of the event subscription protocol, with example
JS code snippets denoting client-side code.

a) The client needs to have logged on to the BMC.
b) The client needs to open a secure websocket with the URL <BMC IP>/subscribe.

```
   var ws = new WebSocket("wss://<BMC IP>/subscribe")
```

c) The client needs to send, over the websocket, a JSON dictionary, comprising
   of key-value pairs. This dictionary serves as the "events filter". All the
   keys are optional, so the dictionary can be empty if no filtering is desired.
   The filters represented by each of the key-value pairs are ORed.

   One of the supported keys is "paths". The corresponding value is an array of
   D-Bus paths. The InterfacesAdded and PropertiesChanged D-Bus signals
   emanating from any of these path(s) alone, and not from any other paths, will
   be included in the event message going out of the BMC.

   The other supported key is "interfaces". The corresponding value is an
   array of D-Bus interfaces. The InterfacesAdded and PropertiesChanged D-Bus
   signal messages comprising of any of these interfaces will be included in
   the event message going out of the BMC.

   All of the following are valid:

   ```
   var data = JSON.stringify(
   {
       "paths": ["/xyz/openbmc_project/logging", "/xyz/openbmc_project/sensors"],
       "interfaces": ["xyz.openbmc_project.Logging.Entry", "xyz.openbmc_project.Sensor.Value"]
   });
   ws.onopen = function() {
       ws.send(data);
   };
   ```

   ```
   var data = JSON.stringify(
   {
       "paths": ["/xyz/openbmc_project/logging", "/xyz/openbmc_project/sensors"],
   });
   ws.onopen = function() {
       ws.send(data);
   };
   ```

   ```
   var data = JSON.stringify(
   {
       "interfaces": ["xyz.openbmc_project.Logging.Entry", "xyz.openbmc_project.Sensor.Value"]
   });
   ws.onopen = function() {
       ws.send(data);
   };
   ```

   ```
   var data = JSON.stringify(
   {
   });
   ws.onopen = function() {
       ws.send(data);
   };
   ```

d) The rest server on the BMC will respond over the websocket when a D-Bus event
   occurs, considering the client supplied filters. The rest servers notifies
   about InterfacesAdded and PropertiesChanged events. The response is a JSON
   dictionary as follows :

   InterfacesAdded
   ```
   "event": InterfacesAdded
   "path": <string : new D-Bus path that was created>
   "intfMap": <dict : a dictionary of interfaces> (similar to org.freedesktop.DBus.ObjectManager.InterfacesAdded )
   ```

   PropertiesChanged
   ```
   "event": PropertiesChanged
   "path": <string : D-Bus path whose property changed>
   "intf": <string : D-Bus interface to which the changed property belongs>
   "propMap": <dict : a dictionary of properties> (similar to org.freedesktop.DBus.Properties.PropertiesChanged)
   ```
