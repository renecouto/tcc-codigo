use std::borrow::Cow;
use std::thread::sleep;
use std::{
    env,
    process,
    time::Duration
};
use futures_util::StreamExt;
extern crate paho_mqtt as mqtt;
use chrono::{DateTime, Utc};
use mqtt::AsyncClient;
use hyper::{Client};
use hyper::client::connect::HttpConnector;

const DFLT_BROKER:&str = "tcp://localhost:1883";

const TOPICS: &[&str] = &["rust/mqtt", "rust/test"];
const QOS: &[i32] = &[1, 1];

fn create_client() -> AsyncClient {

    let host = env::args()
        .nth(1)
        .unwrap_or_else(|| DFLT_BROKER.to_string());
    let create_opts = mqtt::CreateOptionsBuilder::new()
        .server_uri(host)
        .client_id("rust_async_subscribe")
        .finalize();

    // Create the client connection
    let cli = mqtt::AsyncClient::new(create_opts).unwrap_or_else(|e| {
        println!("Error creating the client: {:?}", e);
        process::exit(1);
    });
    cli
}

#[tokio::main]
async fn main() {
    let idb_client = Client::new();
    let mut cli = create_client();
    
    // Get message stream before connecting.
    let mut strm = cli.get_stream(25);

    let lwt = mqtt::Message::new("test", "Async subscriber lost connection", mqtt::QOS_1);

    let conn_opts = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(30))
        .clean_session(false)
        .will_message(lwt)
        .finalize();

    println!("Connecting to the MQTT server...");
    cli.connect(conn_opts).await.unwrap();

    println!("Subscribing to topics: {:?}", TOPICS);
    cli.subscribe_many(TOPICS, QOS).await.unwrap();

    println!("Waiting for messages...");


    while let Some(msg_opt) = strm.next().await {
        if let Some(msg) = msg_opt {
            let d: Cow<'_, str> = msg.payload_str();
            println!("got messages: \n{}", d.len());
            let ms: Vec<ESP32Measurements> = d.split('\n').filter_map(
                |l|{
                if l.trim().len() == 0 {
                    None
                } else {

                
                    let ms: Vec<&str> =  l.split(',').collect();
                    if ms.len() < 3 {
                        println!("got wrong len on message, skipping it: {}", l);
                        None
                    } else {
                        let v1: f64 = ms[0].parse().unwrap();
                        let v2: f64 = ms[1].parse().unwrap();
                        let t: i64 = ms[2].parse().unwrap();
                        use chrono::{TimeZone, Utc};
                        let weather_reading = ESP32Measurements {
                            time: Utc.timestamp_millis(t),
                            variable1: v1,
                            variable2: v2,
                        };
                        if v1.is_nan() || v2.is_nan() {
                            None
                        } else {
                            Some(weather_reading)
                        }
                    }
                }
            }).collect();
            
            
            let mut ret = true;
            while ret {
                ret = send(&idb_client, &ms).await;
                sleep(Duration::from_millis(100));
            }
        

        }
        else {
            println!("Lost connection. Attempting reconnect.");
            while let Err(err) = cli.reconnect().await {
                println!("Error reconnecting: {}", err);
                sleep(Duration::from_millis(1000));
            }
        }
    }
    return;
    
}

struct ESP32Measurements {
    time: DateTime<Utc>,
    variable1: f64,
    variable2: f64,
}

async fn send(client: &Client<HttpConnector>, w: &[ESP32Measurements]) -> bool {
    use hyper::{Body, Method, Request};
    if w.len() == 0 {
        return false;
    }
    let y: String = w.iter().map(|x| format!("test4 variable1={},variable2={} {}\n", x.variable1, x.variable2, x.time.timestamp_millis())).reduce(|x, y| format!("{}{}", x, y)).unwrap();

    let req = Request::builder().method(Method::POST)
        .uri("http://localhost:8084/api/v2/write?org=my-org&bucket=my-bucket&precision=us")
        .header("Authorization", "Token my-token")
        .body(Body::from(y)).unwrap();
    
    let r: Result<hyper::Response<Body>, hyper::Error> = client.request(req).await;
    match r {
        Ok(res) => {
            if !res.status().is_success() {
                println!("failed to get a successful response status! body was: {:?}", hyper::body::to_bytes(res).await.unwrap());
                true
            } else {
                false
            }
        }
        Err(e) => {
            println!("got error: {}", e);
            true
        }
    }
}



