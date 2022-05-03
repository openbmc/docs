mod dbus;

#[tokio::main]
async fn main() {
    let mut conn = dbus::Connection::new(&"test.wltu".to_string());
    let object = conn.register_object(&"/test/wltu".to_string());
    let interface = object.register_interface(&"test.wltu.interface0".to_string());

    interface.add_property(
        &"yes".to_string(),
        dbus::Variant::String("no".to_string()),
        None,
        None,
    );

    interface.add_property_variant(&"maybe".to_string(), dbus::Variant::Integer(21), None, None);

    conn.start().await
}
