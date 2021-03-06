use serde::Deserialize;

#[derive(Deserialize)]
#[serde(tag = "cmd", rename_all = "camelCase")]
pub enum Cmd {
  // your custom commands
  // multiple arguments are allowed
  // note that rename_all = "camelCase": you need to use "myCustomCommand" on JS
  GetStatus {
    callback: String,
    error: String,
  },
  SetLoopback {
    value: bool,
    callback: String,
    error: String,
  },
  SetShouldRemoveNoise {
    value: bool,
    callback: String,
    error: String,
  },
  SetMicrophone {
    value: i32,
    callback: String,
    error: String,
  },
  GetMicrophones {
    callback: String,
    error: String,
  },
  Log {
    msg: String,
    level: i32,
  },
  Exit,
}
// #[derive(Deserialize, Debug)]
// #[serde(tag = "resp", rename_all = "camelCase")]
// pub enum Resp {
//   Ack,
//   Status(bool),
//   Error(String),
//
