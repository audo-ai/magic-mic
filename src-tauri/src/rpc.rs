use crate::cmd::*;
use rand;
use rand::Rng;
use serde::*;
use std::{collections::HashMap, io::*, os::unix::net::UnixStream, string::String, sync::mpsc};

#[derive(Serialize, Deserialize)]
struct JSONRpcReq {
  jsonrpc: String,
  method: String,
  id: String,
  params: Option<serde_json::Value>,
}
#[derive(Serialize, Deserialize)]
struct JSONRpcResp {
  jsonrpc: String,
  id: String,
  result: Option<serde_json::Value>,
  error: Option<serde_json::Value>,
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
    id: get_id(), // TODO make this random
    params: None,
  }
}
fn set_should_remove_noise(b: bool) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setRemoveNoise".to_string(),
    id: get_id(), // TODO make this random
    params: Some(serde_json::Value::Bool(b)),
  }
}
fn get_microphones() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getMicrophones".to_string(),
    id: get_id(), // TODO make this random
    params: None,
  }
}
fn set_microphones(ind: i32) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setMicrophone".to_string(),
    id: get_id(), // TODO make this random
    params: Some(serde_json::Value::Number(serde_json::Number::from(ind))),
  }
}
fn set_loopback(b: bool) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setLoopback".to_string(),
    id: get_id(), // TODO make this random
    //params: Some(serde_json::Value::Bool(serde_json::Bool::from(b)))
    params: Some(serde_json::Value::Bool(b.into())),
  }
}

fn dispatch_and_execute<R: serde::Serialize + Send + 'static>(
  _webview: &mut tauri::WebviewMut,
  mess: R,
  callback: String,
  error: String,
) {
  _webview
    .dispatch(move |wv| {
      tauri::execute_promise_sync(wv, move || Ok(mess), callback, error)
        .expect("expecute_promise_sync should suceed");
    })
    .expect("dispatch should succeed");
}
fn get_rpc_req(event: ServerCmd) -> JSONRpcReq {
  match event {
    ServerCmd::GetStatus {} => get_status(),
    ServerCmd::SetShouldRemoveNoise { value } => set_should_remove_noise(value),
    ServerCmd::SetLoopback { value } => set_loopback(value),
    ServerCmd::GetMicrophones {} => get_microphones(),
    ServerCmd::SetMicrophone { value } => set_microphones(value),
  }
}
fn try_read_to_newline(server: &mut UnixStream, buf: &mut String) {
  loop {
    let mut temp: [u8; 1] = [0; 1];
    let result = server.read(&mut temp);
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

pub fn server_thread(
  mut server: UnixStream,
  rx: mpsc::Receiver<(tauri::WebviewMut, (ServerCmd, String, String))>,
) -> () {
  let mut buf = String::new();
  let mut reqs: HashMap<String, (tauri::WebviewMut, String, String)> = HashMap::new();
  server
    .set_read_timeout(Some(std::time::Duration::from_millis(50)))
    .expect("Can set unix socket read timeout");
  loop {
    try_read_to_newline(&mut server, &mut buf);
    let res: serde_json::Result<JSONRpcResp> = serde_json::from_str(&buf);
    match res {
      Ok(resp) => {
        buf.clear();
        match reqs.get_mut(&resp.id) {
          Some((_webview, callback, error)) => {
            dispatch_and_execute(
              _webview,
              resp.result,
              callback.to_string(),
              error.to_string(),
            );
            reqs.remove(&resp.id);
          }
          None => {
            error!("RPC Response ID not found: {}", resp.id);
          }
        }
      }
      Err(_) => {}
    }

    if let Ok((mut _webview, (event, callback, error))) = rx.try_recv() {
      let rpc_req = get_rpc_req(event);
      // Write to server
      let raw = serde_json::to_string(&rpc_req).unwrap();
      trace!(
        "Server request: \"{}\"",
        serde_json::to_string_pretty(&raw).unwrap()
      );
      server
        .write(&raw.as_bytes())
        .expect("Failed to write to server");
      server.write(b"\n").expect("Failed to write to server");

      // save request
      reqs.insert(rpc_req.id, (_webview, callback, error));
    }
  }
}
