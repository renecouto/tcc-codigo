use std::{
    env,
    process,
    time::Duration
};

extern crate paho_mqtt as mqtt;

const DFLT_BROKER:&str = "tcp://localhost:1883";
const DFLT_CLIENT:&str = "rust_publish";
const DFLT_TOPICS:&[&str] = &["esp32/output", "rust/mqtt", "rust/test"];
const QOS:i32 = 1;

fn main() {
    let host = //.unwrap_or_else(||
        DFLT_BROKER.to_string();
    // );
    let message = env::args().nth(1).unwrap();

    let create_opts = mqtt::CreateOptionsBuilder::new()
        .server_uri(host)
        .client_id(DFLT_CLIENT.to_string())
        .finalize();

    // Create a client.
    let cli = mqtt::Client::new(create_opts).unwrap_or_else(|err| {
        println!("Error creating the client: {:?}", err);
        process::exit(1);
    });

    // Define the set of options for the connection.
    let conn_opts = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(true)
        .finalize();

    // Connect and wait for it to complete or fail.
    if let Err(e) = cli.connect(conn_opts) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }

    let mut msg = mqtt::Message::new(DFLT_TOPICS[0], message.clone(), QOS);
        
    println!("Publishing messages on the {:?} topic", DFLT_TOPICS[0]);
    
    let tok = cli.publish(msg);

    if let Err(e) = tok {
            println!("Error sending message: {:?}", e);
            return;
    }
    let tok = cli.disconnect(None);
    println!("Disconnect from the broker");
    tok.unwrap();
}
