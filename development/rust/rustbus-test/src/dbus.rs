use async_rustbus::rustbus_core;
use async_rustbus::rustbus_core::message_builder::{
    MarshalledMessage, MarshalledMessageBody, MessageBodyParser,
};
use async_rustbus::rustbus_core::standard_messages::{invalid_args, unknown_method};
use async_rustbus::{CallAction, RpcConn};
use rustbus::dbus_variant_sig;
use rustbus_core::message_builder::{MessageBuilder, MessageType};
use std::collections::HashMap;
use std::fmt;
use std::format;
use std::ops::FnMut;
use std::option::Option;
use std::time::UNIX_EPOCH;

/*
Random tests.

busctl call test.wltu /test/wltu org.freedesktop.DBus.Introspectable Introspect

busctl introspect test.wltu /test/wltu
busctl call test.wltu /test/wltu org.freedesktop.DBus.Properties GetAll s test.wltu.interface0
busctl call test.wltu /test/wltu org.freedesktop.DBus.Properties Get ss test.wltu.interface0 yes
busctl call test.wltu /test/wltu org.freedesktop.DBus.Properties Get ss test.wltu.interface0 maybe
busctl call test.wltu /test/wltu org.freedesktop.DBus.Properties Set ssv test.wltu.interface0 maybe t 567
busctl call test.wltu /test/wltu test.wltu.interface0 Test0
busctl call test.wltu /test/wltu test.wltu.interface0 Test1 s "hello"
busctl call test.wltu /test/wltu test.wltu.interface1 Test2
busctl call test.wltu /test/wltu test.wltu.interface1 Test3 t 21

// Expected to fail
busctl call test.wltu /test/wltu org.freedesktop.DBus.Properties Set ssv test.wltu.interface0 yes s no
busctl call test.wltu /test/wltu org.freedesktop.DBus.Properties Set ssv test.wltu.interface0 maybe s no

*/

dbus_variant_sig!(Variant, String => std::string::String; Integer => u64);

impl Clone for Variant {
    fn clone(&self) -> Self {
        match self {
            Variant::String(val) => Variant::String(val.into()),
            Variant::Integer(val) => Variant::Integer(*val),
            Variant::Catchall(_) => Variant::Integer(0),
        }
    }
}

impl Variant {
    fn push_param(&self) -> MarshalledMessageBody {
        let mut body = MarshalledMessageBody::new();
        match self {
            Variant::String(val) => body.push_param(val).unwrap(),
            Variant::Integer(val) => body.push_param(val).unwrap(),
            Variant::Catchall(_) => {}
        }
        body
    }

    fn same(&self, v: &Variant) -> bool {
        match (self, v) {
            (Variant::String(_), Variant::String(_)) => true,
            (Variant::Integer(_), Variant::Integer(_)) => true,
            _ => false,
        }
    }

    fn signature(&self) -> String {
        match self {
            Variant::String(_) => "s".to_string(),
            Variant::Integer(_) => "t".to_string(),
            Variant::Catchall(other) => {
                let mut sig = String::new();
                other.to_str(&mut sig);
                sig
            }
        }
    }
}

pub enum Endpoint {
    Property {
        name: String,
        val: Variant,
        variant: bool,
        getter: Option<Box<dyn Fn()>>,
        setter: Option<Box<dyn FnMut()>>,
    },
    Method {
        name: String,
        input_sig: String,
        output_sig: String,
        method: Box<dyn Fn(&mut MessageBodyParser, &Object) -> MarshalledMessageBody>,
    },
    Signal,
}

// impl Endpoint {
// }

impl fmt::Display for Endpoint {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Endpoint::Property {
                name,
                val,
                variant,
                getter,
                setter,
            } => {
                write!(
                    f,
                    "
<property name=\"{}\" type=\"{}\" access=\"read\">
</property>
                  ",
                    name,
                    if *variant {
                        "v".to_string()
                    } else {
                        val.signature()
                    }
                )
            }
            Endpoint::Method {
                name,
                input_sig,
                output_sig,
                method,
            } => write!(f, ""),
            Endpoint::Signal => write!(f, ""),
        }
    }
}

type Handler = HashMap<String, HashMap<String, HashMap<Option<String>, Box<dyn FnMut()>>>>;
type DbusMap = HashMap<String, Endpoint>;

pub struct Interface {
    pub interface: String,
    pub dbus_map: DbusMap,
}

impl fmt::Display for Interface {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "<interface name=\"{}\">", self.interface)?;

        for endpoint in self.dbus_map.iter() {
            write!(f, "{}", endpoint.1);
        }
        write!(f, "</interface>")
    }
}

impl Interface {
    pub fn new(interface: &String) -> Self {
        return Self {
            interface: interface.into(),
            dbus_map: DbusMap::new(),
        };
    }

    pub fn add_method(
        &mut self,
        func_name: &String,
        input_sig: &String,
        output_sig: &String,
        func: Box<dyn Fn(&mut MessageBodyParser, &Object) -> MarshalledMessageBody>,
    ) {
        self.dbus_map.insert(
            func_name.into(),
            Endpoint::Method {
                name: func_name.into(),
                input_sig: input_sig.into(),
                output_sig: output_sig.into(),
                method: func,
            },
        );
    }
    // pub fn add_signal(&self, interface: &String, path: &String, member: &String, sig: &String) {}
    pub fn add_property_variant(
        &mut self,
        property: &String,
        default_value: Variant,
        getter: Option<Box<dyn Fn()>>,
        setter: Option<Box<dyn FnMut()>>,
    ) {
        self.dbus_map.insert(
            property.into(),
            Endpoint::Property {
                name: property.into(),
                val: default_value,
                variant: true,
                getter: getter,
                setter: setter,
            },
        );
    }

    pub fn add_property(
        &mut self,
        property: &String,
        default_value: Variant,
        getter: Option<Box<dyn Fn()>>,
        setter: Option<Box<dyn FnMut()>>,
    ) {
        self.dbus_map.insert(
            property.into(),
            Endpoint::Property {
                name: property.into(),
                val: default_value,
                variant: false,
                getter: getter,
                setter: setter,
            },
        );
    }
}

// #[derive(Clone)]
pub struct Object {
    pub path: String,
    pub interfaces: HashMap<String, Interface>,
}

impl fmt::Display for Object {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"
        \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">
        <node>
        "
        )?;

        for interface in self.interfaces.iter() {
            write!(f, "{}", interface.1)?;
        }

        write!(f, "</node>")
    }
}

impl Object {
    pub fn new(path: &String) -> Self {
        return Self {
            path: path.into(),
            interfaces: HashMap::new(),
        };
    }

    pub fn register_interface(&mut self, interface: &String) -> &mut Interface {
        self.interfaces
            .insert(interface.into(), Interface::new(interface));
        self.interfaces.get_mut(interface.into()).unwrap()
    }

    fn handler(
        &self,
        parser: &mut MessageBodyParser,
        endpoint: &Endpoint,
    ) -> MarshalledMessageBody {
        match endpoint {
            Endpoint::Property {
                name: _,
                val: _,
                variant: _,
                getter: _,
                setter: _,
            } => MarshalledMessageBody::new(),
            Endpoint::Method {
                name: _,
                input_sig: _,
                output_sig: _,
                method,
            } => (*method)(parser, self),
            Endpoint::Signal => MarshalledMessageBody::new(),
        }
    }

    pub fn process(
        &mut self,
        parser: &mut MessageBodyParser,
        interface: &str,
        member: &str,
    ) -> MarshalledMessageBody {
        let interface = match self.interfaces.get(interface) {
            Some(interface) => Ok(interface),
            None => Err("failed to find interface"),
        };
        match interface {
            Ok(x) => match x.dbus_map.get(member) {
                Some(endpoint) => self.handler(parser, &endpoint),
                None => MarshalledMessageBody::new(),
            },
            Err(e) => MarshalledMessageBody::new(),
        }
    }
}

pub struct Connection {
    connection: String,
    handlers: Handler,
    pub objects: Vec<Object>,
}

impl Connection {
    pub fn new(connection: &String) -> Self {
        return Self {
            connection: connection.into(),
            handlers: Handler::new(),
            objects: Vec::new(),
        };
    }

    pub fn register_object(&mut self, path: &String) -> &mut Object {
        self.objects.push(Object::new(path));
        self.objects.last_mut().unwrap()
    }

    pub async fn start(&mut self) {
        let conn = RpcConn::system_conn(false).await.unwrap();
        conn.insert_call_path("/", CallAction::Intro).await.unwrap();

        // Register all objects
        for object in self.objects.iter_mut() {
            conn.insert_call_path(object.path.as_str(), CallAction::Exact)
                .await
                .unwrap();

            // Introspect
            let interface_intro =
                object.register_interface(&"org.freedesktop.DBus.Introspectable".to_string());
            interface_intro.add_method(
                &"Introspect".to_string(),
                &"".to_string(),
                &"s".to_string(),
                Box::new(|_, object| -> MarshalledMessageBody {
                    let mut body = MarshalledMessageBody::new();
                    body.push_param(format!("{}", object)).unwrap();
                    body
                }),
            );

            let interface_properties =
                object.register_interface(&"org.freedesktop.DBus.Properties".to_string());
            interface_properties.add_method(
                &"GetAll".to_string(),
                &"s".to_string(),
                &"a{sv}".to_string(),
                Box::new(|parser, object| -> MarshalledMessageBody {
                    let interface = parser.get::<String>().unwrap();
                    let mut body = MarshalledMessageBody::new();
                    // match interfaces.get(&interface) {
                    //     Some(properties) => {
                    //         body.push_param(properties).unwrap();
                    //     }
                    //     None => body.push_param(Map::new()).unwrap(),
                    // }
                    body
                }),
            );
        }

        conn.request_name(self.connection.as_str()).await.unwrap();

        loop {
            for object in self.objects.iter_mut() {
                let call = match conn.get_call(object.path.as_str()).await {
                    Ok(c) => c,
                    Err(e) => {
                        eprintln!("Error occurred waiting for calls: {:?}", e);
                        break;
                    }
                };
                assert!(matches!(call.typ, MessageType::Call));

                let mut res = call.dynheader.make_response();
                let parser = &mut call.body.parser();
                res.body = object.process(
                    parser,
                    call.dynheader.interface.as_deref().unwrap(),
                    call.dynheader.member.as_deref().unwrap(),
                );

                match conn.send_msg_wo_rsp(&res).await {
                    Ok(v) => eprintln!("working with version: {:?}", v),
                    Err(e) => eprintln!("error parsing header: {:?}", e),
                }
            }
        }
    }
}

// pub async fn start(&self) {
//     let conn = RpcConn::system_conn(false).await.unwrap();
//     conn.insert_call_path("/test/wltu", CallAction::Exact)
//         .await
//         .unwrap();
//     conn.insert_call_path("/", CallAction::Intro).await.unwrap();
//     conn.request_name("test.wltu").await.unwrap();
//     type Map = HashMap<String, Variant>;
//     let mut interfaces = HashMap::new();
//     let mut property_types = HashMap::new();
//     let mut properties = Map::new();
//     let mut types = HashMap::new();
//     properties.insert("yes".to_string(), Variant::String("798-1364".to_string()));
//     types.insert("yes".to_string(), "s".to_string());
//     properties.insert("maybe".to_string(), Variant::Integer(23));
//     types.insert("maybe".to_string(), "v".to_string());
//     interfaces.insert("test.wltu.interface0".to_string(), properties);
//     property_types.insert("test.wltu.interface0".to_string(), types);
//     loop {
//         let call = match conn.get_call("/test/wltu").await {
//             Ok(c) => c,
//             Err(e) => {
//                 eprintln!("Error occurred waiting for calls: {:?}", e);
//                 break;
//             }
//         };
//         assert!(matches!(call.typ, MessageType::Call));
//         // busctl call example.TimeServer /example/TimeServer org.freedesktop.DBus.Properties Get
//         // busctl call com.google.gbmc.ssd / org.freedesktop.DBus.Introspectable Introspect
//         let res = match (
//             call.dynheader.interface.as_deref().unwrap(),
//             call.dynheader.member.as_deref().unwrap(),
//             call.dynheader.signature.as_deref(),
//         ) {
//             ("test.wltu.interface0", "Test0", _) => {
//                 let mut res = call.dynheader.make_response();
//                 let cur_time = UNIX_EPOCH.elapsed().unwrap().as_millis() as u64;
//                 res.body.push_param(cur_time).unwrap();
//                 res
//             }
//             ("test.wltu.interface0", "Test1", Some("s")) => {
//                 let mut parser = call.body.parser();
//                 let x: String = parser.get::<String>().unwrap();
//                 let mut res = call.dynheader.make_response();
//                 res.body.push_param(format!("body {:?}", x)).unwrap();
//                 res
//             }
//             ("test.wltu.interface0", "Test1", _) => invalid_args(&call.dynheader, None),
//             ("test.wltu.interface1", "Test2", _) => {
//                 let mut res = call.dynheader.make_response();
//                 let v = vec![vec![1, 2], vec![3, 4, 5]];
//                 res.body.push_param(&v).unwrap();
//                 res
//             }
//             ("test.wltu.interface1", "Test3", Some("t")) => {
//                 let mut parser = call.body.parser();
//                 let x: u64 = parser.get::<u64>().unwrap();
//                 let mut res = call.dynheader.make_response();
//                 res.body.push_param(x + 12).unwrap();
//                 res
//             }
//             ("example.TimeServer", "Test3", _) => invalid_args(&call.dynheader, None),
//             ("org.freedesktop.DBus.Properties", "Get", Some("ss")) => {
//                 // todo!("We need to put a introspect impl so that other connection can discover this object.");
//                 let mut res = call.dynheader.make_response();
//                 let mut parser = call.body.parser();
//                 let interface = parser.get::<String>().unwrap();
//                 let property = parser.get::<String>().unwrap();
//                 let _v = "v".to_string();
//                 let property_type = match property_types.get(&interface) {
//                     Some(typesx) => match typesx.get(&property) {
//                         Some(output) => output,
//                         None => &_v,
//                     },
//                     None => &_v,
//                 };
//                 let property = match interfaces.get(&interface) {
//                     Some(properties) => match properties.get(&property) {
//                         Some(output) => Ok(output),
//                         None => Err("Property is not found"),
//                     },
//                     None => Err("Interface is not found"),
//                 };
//                 match property {
//                     Ok(output) => {
//                         if *property_type == _v {
//                             res.body.push_param(output).unwrap();
//                         } else {
//                             res.body = output.push_param();
//                         }
//                         res
//                     }
//                     Err(e) => {
//                         eprintln!("{}", e);
//                         invalid_args(&call.dynheader, Some("ss"))
//                     }
//                 }
//             }
//             ("org.freedesktop.DBus.Properties", "GetAll", Some("s")) => {
//                 let mut res = call.dynheader.make_response();
//                 let mut parser = call.body.parser();
//                 let interface = parser.get::<String>().unwrap();
//                 match interfaces.get(&interface) {
//                     Some(properties) => {
//                         res.body.push_param(properties).unwrap();
//                     }
//                     None => res.body.push_param(Map::new()).unwrap(),
//                 }
//                 res
//             }
//             ("org.freedesktop.DBus.Properties", "Set", Some("ssv")) => {
//                 // todo!("We need to put a introspect impl so that other connection can discover this object.");
//                 let res = call.dynheader.make_response();
//                 let mut parser = call.body.parser();
//                 let interface = parser.get::<String>().unwrap();
//                 let property = parser.get::<String>().unwrap();
//                 let property2 = property.clone();
//                 let value = parser.get::<Variant>().unwrap();
//                 let value2 = value.clone();
//                 let mut changed_properties = Map::new();
//                 let mut changed = false;
//                 let ok = match interfaces.get_mut(&interface) {
//                     Some(properties) => match properties.get(&property) {
//                         Some(output) => {
//                             if output.same(&value) {
//                                 if output != &value {
//                                     changed = true;
//                                     properties.insert(property, value);
//                                     changed_properties.insert(property2, value2);
//                                 }
//                                 Ok(())
//                             } else {
//                                 Err("Set value type does not match existing type")
//                             }
//                         }
//                         None => Err("Property is not found"),
//                     },
//                     None => Err("Interface is not found"),
//                 };
//                 match ok {
//                     Ok(()) => {
//                         if changed {
//                             let mut peroperty_changed_signal = MessageBuilder::new()
//                                 .signal(
//                                     "org.freedesktop.DBus.Properties",
//                                     "PropertiesChanged",
//                                     "/test/wltu",
//                                 )
//                                 .build();
//                             let vec: Vec<String> = vec![];
//                             peroperty_changed_signal.body.push_param(interface).unwrap();
//                             peroperty_changed_signal
//                                 .body
//                                 .push_param(changed_properties)
//                                 .unwrap();
//                             peroperty_changed_signal.body.push_param(vec).unwrap();
//                             conn.send_msg_wo_rsp(&peroperty_changed_signal)
//                                 .await
//                                 .unwrap();
//                         }
//                         res
//                     }
//                     Err(e) => {
//                         eprintln!("{}", e);
//                         invalid_args(&call.dynheader, Some("ssv"))
//                     }
//                 }
//             }
//             ("org.freedesktop.DBus.Introspectable", "Introspect", _) => {
//                 // todo!("We need to put a introspect impl so that other connection can discover this object.");
//                 let mut res = call.dynheader.make_response();
//                 res.body
//                     .push_param(
//                         "
// <!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"
// \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">
// <node>
//  <interface name=\"test.wltu.interface0\">
//   <method name=\"Test0\">
//    <arg name=\"test1\" direction=\"out\" type=\"as\"/>
//   </method>
//   <method name=\"Test1\">
//    <arg name=\"test0\" direction=\"in\" type=\"s\"/>
//    <arg name=\"test1\" direction=\"in\" type=\"s\"/>
//   </method>
//   <property name=\"yes\" type=\"s\" access=\"read\">
//   </property>
//   <property name=\"maybe\" type=\"s\" access=\"readwrite\">
//   </property>
//  </interface>
//  <interface name=\"test.wltu.interface1\">
//   <method name=\"Test2\">
//    <arg name=\"test1\" direction=\"out\" type=\"aai\"/>
//   </method>
//   <method name=\"Test3\">
//    <arg name=\"test0\" direction=\"in\" type=\"t\"/>
//    <arg name=\"value\" direction=\"out\" type=\"t\"/>
//   </method>
//  </interface>
//  <interface name=\"org.freedesktop.DBus.Introspectable\">
//   <method name=\"Introspect\">
//    <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>
//   </method>
//  </interface>
//  <interface name=\"org.freedesktop.DBus.Properties\">
//   <method name=\"Get\">
//    <arg name=\"interface_name\" direction=\"in\" type=\"s\"/>
//    <arg name=\"property_name\" direction=\"in\" type=\"s\"/>
//    <arg name=\"value\" direction=\"out\" type=\"v\"/>
//   </method>
//   <method name=\"GetAll\">
//    <arg name=\"interface_name\" direction=\"in\" type=\"s\"/>
//    <arg name=\"props\" direction=\"out\" type=\"a{sv}\"/>
//   </method>
//   <method name=\"Set\">
//    <arg name=\"interface_name\" direction=\"in\" type=\"s\"/>
//    <arg name=\"property_name\" direction=\"in\" type=\"s\"/>
//    <arg name=\"value\" direction=\"in\" type=\"v\"/>
//   </method>
//   <signal name=\"PropertiesChanged\">
//    <arg type=\"s\" name=\"interface_name\"/>
//    <arg type=\"a{sv}\" name=\"changed_properties\"/>
//    <arg type=\"as\" name=\"invalidated_properties\"/>
//   </signal>
//  </interface>
// </node>
// ",
//                     )
//                     .unwrap();
//                 res
//             }
//             // https://docs.rs/async-rustbus/latest/async_rustbus/rustbus_core/standard_messages/index.html
//             _ => unknown_method(&call.dynheader),
//         };
//         // Check if
//         match conn.send_msg_wo_rsp(&res).await {
//             Ok(v) => eprintln!("working with version: {:?}", v),
//             Err(e) => eprintln!("error parsing header: {:?}", e),
//         }
//     }
// }
