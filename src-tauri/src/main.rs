#![cfg_attr(
  all(not(debug_assertions), target_os = "windows"),
  windows_subsystem = "windows"
)]

use std::{
  env,
  os::unix::net::UnixStream,
  path::PathBuf,
  process::{Command, Stdio},
  sync::mpsc,
  thread,
};
use tauri::api::*;
#[macro_use]
extern crate log;

mod cmd;
mod rpc;
use cmd::*;
use rpc::*;

// https://github.com/tauri-apps/tauri/issues/1308
fn get_real_resource_dir() -> Option<PathBuf> {
  let p = tauri::api::path::resource_dir()?;
  match env::var("APPDIR") {
    Ok(v) => {
      let mut root = PathBuf::from(v);
      if p.has_root() {
        // only on linux here
        root.push(
          p.strip_prefix("/")
            .expect("has_root() returned true; we should be able to strip / prefix"),
        );
      } else {
        root.push(p);
      }
      Some(root)
    }
    Err(_) => Some(p.into()),
  }
}

fn main() {
  env_logger::init();
  info!("Starting");

  let exe_path = env::current_exe().expect("Get current_exe");
  info!("Exe path: {:?}", exe_path.clone().into_os_string());

  // app is either dev or bundled. When it is dev we have to find bins
  // ourselves. Otherwise we can hopefully rely on
  // tauri_api::command::command_path
  let server_path = match env::var("SERVER_PATH") {
    Err(_) => {
      command::command_path(command::binary_command("server".to_string()).unwrap()).unwrap()
    }
    Ok(p) => p,
  };
  trace!("Server Path is: {}", server_path);

  let resource_dir = get_real_resource_dir().expect("resource dir required");

  let mut runtime_lib_path = resource_dir.clone();
  runtime_lib_path.push("native");
  runtime_lib_path.push("runtime_libs");
  info!(
    "Setting LD_LIBRARY_PATH to {:?}",
    runtime_lib_path.clone().into_os_string()
  );

  let mut icon_path = resource_dir.clone();
  icon_path.push("icons");
  icon_path.push("icon.icns");
  info!("icon path: {:?}", icon_path.clone().into_os_string());

  let mut sock_path = tauri::api::path::runtime_dir().expect("get runtime dir");
  sock_path.push("magic-mic.socket");

  info!("Socket path: {:?}", sock_path.clone().into_os_string());

  let stream = match UnixStream::connect(sock_path.clone().into_os_string()) {
    Ok(s) => s,
    Err(e) => {
      info!("Starting new server; error was: {}", e);
      Command::new(server_path)
        .env("LD_LIBRARY_PATH", runtime_lib_path.into_os_string())
        .arg(sock_path.clone().into_os_string())
        .arg(icon_path.into_os_string())
        .arg(exe_path.clone().into_os_string())
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::inherit())
        .spawn()
        .expect("spawn server process");

      // TODO: We have a race condition here because we want the spawned process to
      // start the socket server, so we don't know when its listening
      thread::sleep(std::time::Duration::from_millis(500));
      UnixStream::connect(sock_path.clone().into_os_string())
        .expect("Failed to connect to socket; FIX THIS RACE CONDITON")
    }
  };

  let (to_server, from_main) = mpsc::channel();
  thread::spawn(|| server_thread(stream, from_main));

  tauri::AppBuilder::new()
    .invoke_handler(move |_webview, arg| {
      match serde_json::from_str(arg) {
        Err(e) => Err(e.to_string()),
        Ok(command) => {
          match command {
            JsCmd {
              cmd: Cmd::LocalCommand {
                payload: LocalCmd::Exit,
              },
              ..
            } => Ok(()),
            JsCmd {
              cmd:
                Cmd::LocalCommand {
                  payload: LocalCmd::Log { msg, level },
                },
              ..
            } => {
              // TODO env_logger doesn't seem to print the target. not sure if I
              // am misunderstanding the purpose of target, or if env_logger
              // just doesn't do that or what, but prefixingwith "js: " is my
              // temporary workaround
              match level {
                0 => {
                  debug!(target: "js", "js: {}", msg);
                  Ok(())
                }
                1 => {
                  error!(target: "js", "js: {}", msg);
                  Ok(())
                }
                2 => {
                  info!(target: "js", "js: {}", msg);
                  Ok(())
                }
                3 => {
                  trace!(target: "js", "js: {}", msg);
                  Ok(())
                }
                4 => {
                  // 4=warn (totally intentional)
                  warn!(target: "js", "js: {}", msg);
                  Ok(())
                }
                _ => {
                  warn!(target: "js", "Recieved invalid log level from javsascript");
                  Err("Recieved invalid log level from javsascript".into())
                }
              }
            }
            JsCmd {
              callback: Some(callback),
              error: Some(error),
              cmd: Cmd::ExternalCommand { payload },
            } => to_server
              .send((_webview.as_mut(), (payload, callback, error)))
              .map_err(|e| format!("sending error: {}", e)),
            JsCmd { .. } => Err("JsCmd found missing callback or error".into()),
          }
        }
      }
    })
    .build()
    .run();
  info!("Exiting");
}
