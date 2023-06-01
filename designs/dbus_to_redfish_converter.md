# DBus To Redfish Converter 

Author: Abhilash Raju 

Other contributors: 

Created: May 28, 2023 

## Problem Description

BmcWeb is an embedded web server which serves endpoints for external clients to set and get device configurations. BmcWeb in turn depends on  other system demons to serve the client requests. Firmware uses DBus messaging IPC to communicate between demons and BmcWeb. BmcWeb uses payloads based on redfish schema for communicating with external clients. Thus, we need a conversion layer that can convert the DBus properties to Redfish schema. The implementation of the conversion layer should address following challenges.
  ###  1. DBus Tree To Redfish Data Mapping 
  There is no one-to-one correspondence between DBus tree structure and Redfish data. Often Dbus property name and reddish attributes names are different. The datatypes of properties can be different. Often property values may undergo some kind transformations before assigned  to target redfish attributes.
  ### 2. Filtering 
  Not all DBus Interface need to be exposed. Not all properties of a given interface need to be exposed. That necessitate application of filters in the conversion layers  to give out just the data needed by the redfish schema.
  ### 3. Optional Properties
  Redfish schema allows optional properties. Supporting optional properties are left to the OEM's decision. Being a generic component BmcWeb need some mechanism to support  variations in the implementation of optional properties. 

  Apart from the functional requirements, there are other non-functional requirement we should take care.

  ### 1) Modularity

 Often we should implement non trivial conversion logic to support certain properties. So it is better to isolate the conversions of individual properties from rest of the parsing process.

  ### 2) Locality Of Reasoning 

  It is easier to understand and maintain implementation that exhibits better locality of reasoning . We need not jump around several functions or files to understand complete conversion process.

  ### 3) Scalability

  It should be easy to add new properties with minimal impact in the overall parsing process. A bug in the newly added property should have minimal, if not no, impact on other properties. At max the newly added buggy code should  result in just the absence of the new property in response payload. 


## Background and References


## Requirements

 At present BmcWeb addresses the functional challenges [1](#1-DBus-Tree-To-Redfish-Data-Mapping) and [2](#L63) mentioned in problem description in an imperative style. The solution to the functional challenge [3](#L65) (if any) is not evident anywhere in  the current implementation. It is easy to see multi page long DBus to Redfish conversion functions throughout BmcWeb implementation. The functions are implemented using highly nested loops and conditional statements, which is often hard to read and comprehend. The intent of the code is not easily visible from these highly nested scopes ([ See here for example](https://github.com/openbmc/bmcweb/blob/5db7dfd6278258fd069ae29881920e365a533547/redfish-core/lib/ethernet.hpp#L185 ) ). The implementation is very much repetitive. We need a better solution to solve all these problems. The proposed design is an attempt to fix the problems outlined here. 
 

## Proposed Design


The new APIs proposed below will help developers just state their intent in a declarative style. It is often as simple as just stating "map xxx dbus property to yyy redfish attribute" .   

The following code snippet will demonstrate these capabilities.

The current code snippet to convert MacAdress from Dbus property to Json attribute look like this in imperative style
```
for (const auto& objpath : dbusData)
    {
        for (const auto& ifacePair : objpath.second)
        {
            if (objpath.first == "/xyz/openbmc_project/network/" + ethifaceId)
            {
                
                if (ifacePair.first == "xyz.openbmc_project.Network.MACAddress")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "MACAddress")
                        {
                            const std::string* mac =
                                std::get_if<std::string>(&propertyPair.second);
                            if (mac != nullptr)
                            {
                                ethData.macAddress = *mac;
                            }
                        }
                    }
                }
            }
        }
    }

    //somewhere else
    jsonResponse["MACAddress"] = ethData.macAddress;
```
In our new declarative style it can be reduced to as follows

```
addInterfaceHandler("xyz.openbmc_project.Network.MACAddress","MACAddress",mapToKey("MACAddress"));
```

We can argue that the above case is a trivial one . At times the conversions are not so trivial as above . You may require complex conversion logic.
Don't worry, we can  solve that too.
addInterfaceHandler can accept user written conversion logic as well.
What you have to do is just use mapToHandler instead mapToKey. See example below

Before
```
for (const auto& objpath : dbusData)
    {
        for (const auto& ifacePair : objpath.second)
        {
            if (objpath.first == "/xyz/openbmc_project/network/" + ethifaceId)
            {
                
                if (ifacePair.first == "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "DefaultGateway")
                        {
                            const std::string* defaultGateway =
                                std::get_if<std::string>(&propertyPair.second);
                            if (defaultGateway != nullptr)
                            {
                                std::string defaultGatewayStr = *defaultGateway;
                                if (defaultGatewayStr.empty())
                                {
                                    ethData.defaultGateway = "0.0.0.0";
                                }
                                else
                                {
                                    ethData.defaultGateway = defaultGatewayStr;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
```
After
```
addInterfaceHandler("xyz.openbmc_project.Network.EthernetInterface",
                  "DefaultGateway", mapToHandler<std::string>([](auto defaultGatewayStr ){
                        nlohmann::json jsonResponse;
                        
                        jsonResponse["IPv6DefaultGateway"] =defaultGatewayStr.empty()?"0.0.0.0":defaultGatewayStr;
                        return jsonResponse;
              }));
```

Sometime a Dbus property may be converted into a deeply nested json attribute. You can use above mapToHandler technique in complicated cases. In a trivially nesting scenario you can use following technique using mapToKey. All you have to do is just add target attribute key as '/' separated path 

For eg: The json snippet {"NICEnabled":{"Status":{ "State":true }}} can be generated from following code snippet.
```
 addInterfaceHandler("xyz.openbmc_project.Network.EthernetInterface","NICEnabled",mapToKey("NICEnabled/Status/State"));
```

In most cases the conversion is taken care by mapToKey default handler. In case if you want to be explicit about the datatype you can give it as an explicit template argument as below
```
 addInterfaceHandler("xyz.openbmc_project.Network.EthernetInterface","NICEnabled",mapToKey<bool>("NICEnabled/Status/State"));
```

At times we need to convert a dbus property based on the object path or the interface it belonged, you can use specialised version of mapToHandler API. This will be the case when mutiple objects implements same interface or multiple interfaces exposes same property names. 

```
addInterfaceHandler("xyz.openbmc_project.State.Decorator.Availability",
                "Available",
                mapToHandler<std::vector<uint32_t>>(
                    [](auto path, auto ifaceName, auto val) {
                      nlohmann::json j;
                      if (path == "/xyz/openbmc_project/license/entry1/")
                      {
                          j["Some Path"]["Availability"] = val;
                          return j;
                      }
                      j["Other Path"]["Availability"] = "Converted Value";
                      return j;
                }));
```
Notice the call back signature. You should give a handler that accepts path and interface name apart from the value parameter.

### Two Stages of Parser

In the proposed Design there are two stages for setting up a conversion routine. 

### 1) Declaration of conversion handlers
  Here you will write the mapping handlers outlined above.Typically this can be done in global scope as conversion handler object. This object will be created only once. It does not depend on the number of invocation of get request.

### 2) Evaluation of conversion handlers

  The evaluation of conversion handlers happens during the get request invocation. The evaluation will result in success or failure. If it succeeded , then you will get a resulting Json conforming to redfish schema. You can add extra attributes that were not available from the Dbus interfaces at this stage. The final json can be sent back as response payload.
  The following code snippet shows  the evaluation stage implementation , which is usually written in the  get request handler body.
  ```
  DbusTreeParser parser(resp, getGlobalEthernetHandlers(), true);

  nlohmann::json result;
  parser.onSuccess(
          [ &result](DbusParserStatus status, auto&& summary) {
              if (status == DbusParserStatus::Failed)
              {
                  return;
              }
              result = summary;
      })
      .parse();
  
  std::cout<<result.dump(4);
  ```

  The complete structure of the code is as follows
  ```
using namespace redfish;

struct EthIfaceConvHandlers : DbusBaseHandler
{
    
    EthIfaceConvHandlers()
    {
        
        addInterfaceHandler("xyz.openbmc_project.Network.MACAddress","MACAddress",mapToKey("MACAddress"));

        addInterfaceHandler("xyz.openbmc_project.Network.EthernetInterface",
            "DefaultGateway", mapToHandler<std::string>([](auto defaultGatewayStr ){
                    nlohmann::json jsonResponse;
                    
                    jsonResponse["IPv6DefaultGateway"] =defaultGatewayStr.empty()?"0.0.0.0":defaultGatewayStr;
                    return jsonResponse;
        }));
        addInterfaceHandler("xyz.openbmc_project.Network.EthernetInterface","NICEnabled",mapToKey("Status/State"));
            
            
        
    }
    static EthIfaceConvHandlers& getGlobalEthernetHandlers(){
        static EthIfaceConvHandlers gHandler;
        return gHandler;
    }
};
inline void requestEthernetInterfacesRoutes(App& app){

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::getEthernetInterface)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& ifaceId) {
        
    
            sdbusplus::message::object_path path("/xyz/openbmc_project/network");
            dbus::utility::getManagedObjects(
                "xyz.openbmc_project.Network", path,
                [](
                    const boost::system::error_code& errorCode,
                    const dbus::utility::ManagedObjectType& resp) {
            
                if (errorCode)
                {
                    //handler dbus error;
                    return;
                }
            
                DbusTreeParser parser(resp, EthIfaceConvHandlers::getGlobalEthernetHandlers(), true);
                
                nlohmann::json result;
                parser.onSuccess(
                        [ &result](DbusParserStatus status, auto&& summary) {
                            if (status == DbusParserStatus::Failed)
                            {
                                return;
                            }
                            //summary is redfish compatible json 
                            asyncResp->jsonResponse=summary;
                    })
                    .parse();
                
                
                });
                
            
            }
}
    
```

You can configure the DbusTreeParser with an optional filter handler. Through filter we can suggest the parser about the objects we are interested in for parsing. 
We can set the strictness of the parser. If the handler table contains unknown properties then by-default parsing fails. But you can change this behaviour by suggesting parser to ignore the unknown properties. This way we can solve the optional redfish property handling. OEM are free to implement their own DBus interfaces supporting only the mandatory fields required. 

## Alternatives Considered

See the requirement section to see the problems in current approach.

## Impacts

### API impact?
This is a different style programming to achieve same result.So there will be some minimal learnings involved in using new API. 
### Security impact? 
 None
 ### Documentation impact? 
 New APIs need to be documented and example usage should be available. May be this design doc can serve as API documentation.

 ### Performance impact? 
 It is too early to say any thing about performance. We need to measure to see the difference.APIs are designed with utmost care to avoid any costly repetitive operations. The first stage of conversion process will happen only once. So most of performance will depends on how fast the evaluation( the second stage ) is running.

### Developer impact? 
API are designed to help in developer productivity. May be initial leaning curve is there. But in long run BmcWeb will have better readable code.



## Testing

Unit Test cases are added to test the library features. Test uses mocked Dbus tree content. Test verifies if the generated json from the converter is as per expectation.  
