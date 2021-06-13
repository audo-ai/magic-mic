use crate::cmd::*;
use rand;
use rand::Rng;
use serde::*;
use std::{collections::HashMap, io::*, string::String};
use tokio::{
  net::UnixStream,
  sync::{mpsc, oneshot::Sender},
};

#[derive(Serialize, Deserialize)]
pub struct JSONRpcReq {
  pub jsonrpc: String,
  pub method: String,
  pub id: String,
  pub params: Option<serde_json::Value>,
}
#[derive(Serialize, Deserialize)]
pub struct JSONRpcResp {
  pub jsonrpc: String,
  pub id: String,
  pub result: Option<serde_json::Value>,
  pub error: Option<serde_json::Value>,
}

fn get_id() -> String {
  rand::thread_rng()
    .sample_iter(&rand::distributions::Alphanumeric)
    .take(7)
    .map(char::from)
    .collect()
}

fn get_status() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getStatus".to_string(),
    id: get_id(),
    params: None,
  }
}
fn set_should_remove_noise(b: bool) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setRemoveNoise".to_string(),
    id: get_id(),
    params: Some(serde_json::Value::Bool(b)),
  }
}
fn get_microphones() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getMicrophones".to_string(),
    id: get_id(),
    params: None,
  }
}
fn set_microphones(ind: i32) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setMicrophone".to_string(),
    id: get_id(),
    params: Some(serde_json::Value::Number(serde_json::Number::from(ind))),
  }
}
fn get_remove_noise() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getRemoveNoise".to_string(),
    id: get_id(),
    params: None,
  }
}
fn get_loopback() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getLoopback".to_string(),
    id: get_id(),
    params: None,
  }
}
fn set_loopback(b: bool) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setLoopback".to_string(),
    id: get_id(),
    //params: Some(serde_json::Value::Bool(serde_json::Bool::from(b)))
    params: Some(serde_json::Value::Bool(b.into())),
  }
}
fn get_audio_processors() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getProcessors".to_string(),
    id: get_id(),
    params: None,
  }
}
fn set_audio_processor(ind: i32) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setProcessor".to_string(),
    id: get_id(),
    params: Some(serde_json::Value::Number(serde_json::Number::from(ind))),
  }
}
fn get_rpc_req(event: ServerCmd) -> JSONRpcReq {
  match event {
    ServerCmd::GetStatus {} => get_status(),
    ServerCmd::SetShouldRemoveNoise { value } => set_should_remove_noise(value),
    ServerCmd::SetLoopback { value } => set_loopback(value),
    ServerCmd::GetMicrophones {} => get_microphones(),
    ServerCmd::SetMicrophone { value } => set_microphones(value),
    ServerCmd::GetLoopback {} => get_loopback(),
    ServerCmd::GetRemoveNoise {} => get_remove_noise(),
    ServerCmd::GetProcessors {} => get_audio_processors(),
    ServerCmd::SetProcessor { value } => set_audio_processor(value),
  }
}
fn try_read_to_newline(server: &mut UnixStream, buf: &mut String) {
  loop {
    let mut temp: [u8; 1] = [0; 1];
    let result = server.try_read(&mut temp);
    match result {
      Ok(0) => {
        panic!("Server closed. We should recover from this though");
      }
      Ok(_) => {
        if temp[0] == '\n' as u8 {
          break;
        } else {
          buf.push(temp[0] as char);
        }
      }
      Err(_) => {
        break;
      }
    };
  }
}

// TODO: I could probably rewrite this cleaner now that I can use tokio
pub async fn server_thread(
  mut server: UnixStream,
  mut rx: mpsc::Receiver<(ServerCmd, Sender<JSONRpcResp>)>,
) -> () {
  let mut buf = String::new();
  let mut reqs: HashMap<String, Sender<JSONRpcResp>> = HashMap::new();
  loop {
    // rustfmt doesn't know how to format this macro
    #[rustfmt::skip]
    tokio::select! {
      _ = server.readable() => {
	try_read_to_newline(&mut server, &mut buf);
	let res: serde_json::Result<JSONRpcResp> = serde_json::from_str(&buf);
	match res {
	  Ok(resp) => {
	    trace!(
	      "Server response: \"{}\"",
	      serde_json::to_string_pretty(&resp).unwrap()
	    );
	    buf.clear();
	    match reqs.remove(&resp.id) {
	      Some(send) => {
		send.send(resp);
	      }
	      None => {
		error!("RPC Response ID not found: {}", resp.id);
	      }
	    }
	  }
	  Err(_) => {}
	}
      }
      Some((event, send)) = rx.recv() => {
	let rpc_req = get_rpc_req(event);
	// Write to server
	let raw = serde_json::to_string(&rpc_req).unwrap();
	trace!(
	  "Server request: \"{}\"",
	  serde_json::to_string_pretty(&raw).unwrap()
	);
	server.writable().await.expect("server is open");
	server
	  .try_write(&raw.as_bytes())
	  .expect("Failed to write to server");
	server.try_write(b"\n").expect("Failed to write to server");

	// save request
	reqs.insert(rpc_req.id, send);
      }
    }
  }
}
