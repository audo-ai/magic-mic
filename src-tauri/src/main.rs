#![cfg_attr(
  all(not(debug_assertions), target_os = "windows"),
  windows_subsystem = "windows"
)]

use std::{
  env,
  path::PathBuf,
  process::{Command, Stdio},
  thread::spawn,
};
use tauri::api::*;
#[macro_use]
extern crate log;

use tokio::{
  net::UnixStream,
  runtime::Handle,
  sync::{mpsc, oneshot},
  time::{sleep, Duration},
};

mod cmd;
mod rpc;
use cmd::*;
use rpc::*;

#[tokio::main]
async fn main() -> () {
  env_logger::init();
  info!("Starting");

  let ctx = tauri::generate_context!();

  let exe_path = process::current_binary().expect("Get current_exe");
  info!("Exe path: {:?}", exe_path.clone().into_os_string());

  let resource_dir =
    tauri::api::path::resource_dir(ctx.package_info()).expect("resource dir required");

  let mut runtime_lib_path = resource_dir.clone();
  runtime_lib_path.push("native");
  runtime_lib_path.push("runtime_libs");
  info!(
    "runtime lib path: {:?}",
    runtime_lib_path.clone().into_os_string()
  );

  let mut preload_lib_path = runtime_lib_path.clone();
  preload_lib_path.push("preload");
  info!(
    "preload lib path: {:?}",
    preload_lib_path.clone().into_os_string()
  );

  let mut audio_processor_path = runtime_lib_path.clone();
  audio_processor_path.push("audioprocs");
  info!(
    "audio processor path: {:?}",
    audio_processor_path.clone().into_os_string()
  );

  let mut icon_path = resource_dir.clone();
  icon_path.push("icons");
  icon_path.push("icon.icns");
  info!("icon path: {:?}", icon_path.clone().into_os_string());

  let mut sock_path = tauri::api::path::runtime_dir().expect("get runtime dir");
  sock_path.push("magic-mic.socket");

  info!("Socket path: {:?}", sock_path.clone().into_os_string());

  let stream = match UnixStream::connect(sock_path.clone().into_os_string()).await {
    Ok(s) => s,
    Err(e) => {
      info!("Starting new server; error was: {}", e);
      let args: Vec<String> = vec![
        sock_path
          .clone()
          .into_os_string()
          .into_string()
          .expect("sock_path valid string"),
        icon_path
          .into_os_string()
          .into_string()
          .expect("icon_path valid string"),
        exe_path
          .clone()
          .into_os_string()
          .into_string()
          .expect("exe_path valid string"),
        audio_processor_path
          .clone()
          .into_os_string()
          .into_string()
          .expect("audio_processor_path valid string"),
      ];
      process::Command::new_sidecar("server")
	.expect("Create server command struct")
	.envs(
	  [(
	    "LD_LIBRARY_PATH".into(),
	    preload_lib_path.to_str().expect("No unicode chars").into(),
	  )]
	    .iter() // convert to hashmap
	    .cloned()
	    .collect(),
	)
	.args(args.into_iter())
	.spawn()
	.expect("spawn server process");

      // TODO: We have a race condition here because we want the spawned process to
      // start the socket server, so we don't know when its listening
      sleep(Duration::from_millis(500)).await;
      UnixStream::connect(sock_path.clone().into_os_string())
        .await
        .expect("Failed to connect to socket; FIX THIS RACE CONDITON")
    }
  };

  let (to_server, from_main) = mpsc::channel(32);
  let handle = Handle::current();
  spawn(move || {
    handle.spawn(async move { server_thread(stream, from_main).await });
  });

  tauri::Builder::default()
    .manage(to_server)
    .invoke_handler(tauri::generate_handler![
      cmd::getStatus,
      cmd::getLoopback,
      cmd::getRemoveNoise,
      cmd::setLoopback,
      cmd::setShouldRemoveNoise,
      cmd::setMicrophone,
      cmd::getMicrophones,
      cmd::getProcessors,
      cmd::setProcessor,
      cmd::jsLog,
    ])
    .run(ctx)
    .expect("Error while running tauri app");

  info!("Exiting");

  ()
}
