use serde::*;

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
  "a".to_string()
}

pub fn get_status() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getStatus".to_string(),
    id: get_id(), // TODO make this random
    params: None
  }
}
pub fn set_should_remove_noise(b: bool) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setRemoveNoise".to_string(),
    id: get_id(), // TODO make this random
    params: Some(serde_json::Value::Bool(b))
  }
}
pub fn get_microphones() -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "getMicrophones".to_string(),
    id: get_id(), // TODO make this random
    params: None
  }
}
pub fn set_microphones(ind: i32) -> JSONRpcReq {
  JSONRpcReq {
    jsonrpc: "2.0".to_string(),
    method: "setMicrophone".to_string(),
    id: get_id(), // TODO make this random
    params: Some(serde_json::Value::Number(serde_json::Number::from(ind)))
  }
}
