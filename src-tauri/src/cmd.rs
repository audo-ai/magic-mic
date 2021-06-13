use serde::Deserialize;
use tokio::sync::{mpsc, oneshot};

#[derive(Deserialize)]
#[serde(tag = "cmd", rename_all = "camelCase")]
pub enum ServerCmd {
  GetStatus,
  GetLoopback,
  GetRemoveNoise,
  SetLoopback { value: bool },
  SetShouldRemoveNoise { value: bool },
  SetMicrophone { value: i32 },
  GetMicrophones,
  GetProcessors,
  SetProcessor { value: i32 },
}
#[derive(Deserialize)]
#[serde(tag = "cmd", rename_all = "camelCase")]
pub enum LocalCmd {
  Log { msg: String, level: i32 },
  Exit,
}

fn rpc_error_string(err: crate::rpc::JSONRpcResp) -> String {
  // it should eventually give a meaningul string for an error and also if the rpcresp isn't actually an error say something like "handle_rror calle but doesn't appear to be error"
  "handle_error not implemented yet".into()
}

#[tauri::command]
pub async fn getStatus(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
) -> Result<bool, String> {
  let (tx, rx) = oneshot::channel();
  state.send((ServerCmd::GetStatus, tx)).await;

  match rx.await {
    Ok(crate::rpc::JSONRpcResp {
      result: Some(serde_json::Value::Bool(b)),
      error: None,
      ..
    }) => Ok(b),
    Ok(bad) => Err(rpc_error_string(bad)),
    Err(e) => {
      error!("tokio oneshot rx await error in getStatus, {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn getLoopback(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
) -> Result<bool, String> {
  let (tx, rx) = oneshot::channel();
  state.send((ServerCmd::GetLoopback, tx)).await;

  match rx.await {
    Ok(crate::rpc::JSONRpcResp {
      result: Some(serde_json::Value::Bool(b)),
      error: None,
      ..
    }) => Ok(b),
    Ok(bad) => Err(rpc_error_string(bad)),
    Err(e) => {
      error!("tokio oneshot rx await error getLoopback {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn getRemoveNoise(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
) -> Result<bool, String> {
  let (tx, rx) = oneshot::channel();
  state.send((ServerCmd::GetRemoveNoise, tx)).await;

  match rx.await {
    Ok(crate::rpc::JSONRpcResp {
      result: Some(serde_json::Value::Bool(b)),
      error: None,
      ..
    }) => Ok(b),
    Ok(bad) => Err(rpc_error_string(bad)),
    Err(e) => {
      error!("tokio oneshot rx await error in getRemoveNoise {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn setShouldRemoveNoise(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
  value: bool,
) -> Result<(), String> {
  let (tx, rx) = oneshot::channel();
  state
    .send((ServerCmd::SetShouldRemoveNoise { value: value }, tx))
    .await;

  match rx.await {
    Ok(_) => Ok(()),
    Err(e) => {
      error!("tokio oneshot rx await error in setShouldRemoveNoise {}", e);
      Err("Internal rust error".into())
    }
  }
}

#[tauri::command]
pub async fn getProcessors(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
) -> Result<serde_json::Value, String> {
  let (tx, rx) = oneshot::channel();
  state.send((ServerCmd::GetProcessors, tx)).await;

  match rx.await {
    Ok(crate::rpc::JSONRpcResp {
      result: Some(serde_json::Value::Object(obj)),
      error: None,
      ..
    }) => Ok(serde_json::Value::Object(obj)),
    Ok(bad) => Err(rpc_error_string(bad)),
    Err(e) => {
      error!("tokio oneshot rx await error in getProcessors {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn setProcessor(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
  value: i32,
) -> Result<(), String> {
  let (tx, rx) = oneshot::channel();
  state
    .send((ServerCmd::SetProcessor { value: value }, tx))
    .await;

  match rx.await {
    Ok(_) => Ok(()),
    Err(e) => {
      error!("tokio oneshot rx await error in setProcessor {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn getMicrophones(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
) -> Result<serde_json::Value, String> {
  let (tx, rx) = oneshot::channel();
  state.send((ServerCmd::GetMicrophones, tx)).await;

  match rx.await {
    Ok(crate::rpc::JSONRpcResp {
      result: Some(serde_json::Value::Object(outer)),
      error: None,
      ..
    }) => {
      // TODO validate this
      Ok(serde_json::Value::Object(outer))
    }
    Ok(bad) => Err(rpc_error_string(bad)),
    Err(e) => {
      error!("tokio oneshot rx await error in getMicrophones {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn setLoopback(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
  value: bool,
) -> Result<(), String> {
  let (tx, rx) = oneshot::channel();
  state
    .send((ServerCmd::SetLoopback { value: value }, tx))
    .await;

  match rx.await {
    Ok(_) => Ok(()),
    Err(e) => {
      error!("tokio oneshot rx await error in setLoopback {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub async fn setMicrophone(
  state: tauri::State<'_, mpsc::Sender<(ServerCmd, oneshot::Sender<crate::rpc::JSONRpcResp>)>>,
  value: i32,
) -> Result<(), String> {
  let (tx, rx) = oneshot::channel();
  state
    .send((ServerCmd::SetMicrophone { value: value }, tx))
    .await;

  match rx.await {
    Ok(_) => Ok(()),
    Err(e) => {
      error!("tokio oneshot rx await error in setMicrophone {}", e);
      Err("Internal rust error".into())
    }
  }
}
#[tauri::command]
pub fn jsLog(msg: String, level: i32) -> Result<(), String> {
  match level {
    0 => {
      debug!("js log: {}", msg);
    }
    1 => {
      error!("js log: {}", msg);
    }
    2 => {
      info!("js log: {}", msg);
    }
    3 => {
      trace!("js log: {}", msg);
    }
    4 => {
      warn!("js log: {}", msg);
    }
    a => {
      error!("js attempted to log at level {}", a)
    }
  }
  Ok(())
}
