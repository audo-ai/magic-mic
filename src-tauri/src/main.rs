#![cfg_attr(
  all(not(debug_assertions), target_os = "windows"),
  windows_subsystem = "windows"
)]

use std::{
  string::String,
  io::*,
  env,
  process::{
    Command,
    Stdio
  },
  thread,
  sync::mpsc,
  os::unix::net::{
    UnixListener
  },
  fs::*
};
use tauri::api::*;
use anyhow;

mod cmd;
mod rpc;
use cmd::*;
use rpc::*;

fn get_message<R: Read>(r: &mut R) -> JSONRpcResp {
  let mut str = String::from("");
  let mut arr = [0; 256];
  loop {
    let res = r.read(&mut arr).expect("Read should succeed");
    if res != 0 {
    }
    for i in 0..res {
      str.push(arr[i] as char);
    }
    if res != 0 && arr[res - 1] == 10 {
      break;
    }
  }
  serde_json::from_str(&str).unwrap()
}
fn rxtx_server<T: Read + Write>(server: &mut T, req: JSONRpcReq) -> JSONRpcResp {
  let r = serde_json::to_string(&req).unwrap();
  println!("Server request: {}", serde_json::to_string(&r).unwrap());
  server.write(&r.as_bytes()).expect("Failed to write to server");
  server.write(b"\n").expect("Failed to write to server");
  let r = get_message(server);
  println!("Server response: {}", serde_json::to_string(&r).unwrap());
  return r
}
fn server_thread<T: Read + Write>(mut server: T, rx: mpsc::Receiver<(tauri::WebviewMut, Cmd)>) -> () {
  loop {
    if let Ok((mut _webview, event)) = rx.recv() {
      match event {
	Cmd::GetStatus{callback, error} => {
	  match rxtx_server(&mut server,  get_status()) {
	    JSONRpcResp{result: Some(serde_json::Value::Bool(b)), ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(b), callback, error);
	      });
	    }
	    JSONRpcResp{error: Some(e), ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(e), callback, error);
	      });
	    }
	    _ => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(String::from("Unknown error")), callback, error);
	      });
	    }
	  };
	},
	Cmd::SetShouldRemoveNoise{value, callback, error} => {
	  // let r = serde_json::to_string(&set_should_remove_noise(value)).unwrap();
	  // println!("Server request: {}", serde_json::to_string(&r).unwrap());
	  // server.write(&r.as_bytes());
	  // server.write(b"\n");
	  // let r = get_message(& mut server);
	  // println!("Server response: {}", serde_json::to_string(&r).unwrap());
	  match rxtx_server(&mut server, set_should_remove_noise(value)) {
	    JSONRpcResp{result: None, ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(serde_json::Value::Null), callback, error);
	      });
	    }
	    JSONRpcResp{error: Some(e), ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(e), callback, error);
	      });
	    }
	    _ => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(String::from("Unknown error")), callback, error);
	      });
	    }
	  };
	},
	Cmd::GetMicrophones {callback, error} => {
	  match rxtx_server(&mut server, get_microphones()) {
	    JSONRpcResp{result: Some(serde_json::Value::Array(v)), ..} => {
	      // Todo verify v is of correct form
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(v), callback, error);
	      });
	    }
	    JSONRpcResp{result: Some(e), ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| {
		  Ok(format!("Unexpected result: {}", e))
		}, callback, error);
	      });
	    }
	    JSONRpcResp{error: Some(e), ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(e), callback, error);
	      });
	    }
	    _ => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(String::from("Unknown error")), callback, error);
	      });
	    }
	  };
	},
	Cmd::SetMicrophone {value, callback, error} => {
	  match rxtx_server(&mut server, set_microphones(value)) {
	    JSONRpcResp{error: None, ..} => {
	      // Todo verify v is of correct form
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(serde_json::Value::Null), callback, error);
	      });
	    }
	    JSONRpcResp{error: Some(e), ..} => {
	      _webview.dispatch(move|wv| {
		tauri::execute_promise_sync(wv, move|| Ok(e), callback, error);
	      });
	    }
	  };
	},
	Cmd::Exit => {
	  break;
	}
      }
    }
  }
}

fn main() {
  // app is either dev or bundled. When it is dev we have to find bins
  // ourselves. Otherwise we can hopefully rely on
  // tauri_api::command::command_path
  let server_path = match env::var("SERVER_PATH") {
    Err(_) => command::command_path("server".to_string()).unwrap(),
    Ok(p) => p,
  };
  println!("Server Path is: {}", server_path);

  let mut sock_path = tauri::api::path::runtime_dir().expect("get runtime dir");
  sock_path.push("magic-mic.socket");


  if metadata(sock_path.clone().into_os_string()).is_ok() {
    remove_file(sock_path.clone().into_os_string()).expect("Failed to remove socket path, maybe permissions?");
  }
  let listener = UnixListener::bind(sock_path.clone().into_os_string()).expect("Couldn't bind to unix socket");

  Command::new(server_path)
    .arg(sock_path.clone().into_os_string())
    .stdin(Stdio::piped())
    .stdout(Stdio::piped())
    .stderr(Stdio::inherit())
    .spawn()
    .expect("spawn server process");

  let stream = listener.incoming().next().expect("Couldn't listen on socket").expect("Child failed to connect to socket");

  let (to_server, from_main) = mpsc::channel();
  thread::spawn(|| { server_thread(stream, from_main) });

  tauri::AppBuilder::new()
    .invoke_handler(move |_webview, arg| {
      match serde_json::from_str(arg) {
	Err(e) => {
	  Err(e.to_string())
	}
	Ok(command) => {
	  match command {
	    c @ Cmd::GetStatus{..} => {
	      to_server.send((_webview.as_mut(), c)).map_err(|e| format!("sending error: {}", e))
	    },
	    c @ Cmd::SetShouldRemoveNoise{..} => {
	      to_server.send((_webview.as_mut(), c)).map_err(|e| format!("sending error: {}", e))
	    },
	    c @ Cmd::GetMicrophones{..} => {
	      to_server.send((_webview.as_mut(), c)).map_err(|e| format!("sending error: {}", e))
	    },
	    c @ Cmd::SetMicrophone{..} => {
	      to_server.send((_webview.as_mut(), c)).map_err(|e| format!("sending error: {}", e))
	    },
	    Cmd::Exit => {
	      Ok(())
	    }
	  }
	}
      }
    })
    .build()
    .run();
  println!("Exiting");
}
