use serde::Deserialize;

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct JsCmd {
  // your custom commands
  // multiple arguments are allowed
  // note that rename_all = "camelCase": you need to use "myCustomCommand" on JS
  #[serde(flatten)]
  pub cmd: Cmd,
  pub callback: Option<String>,
  pub error: Option<String>,
}
#[derive(Deserialize)]
#[serde(tag = "cmd", rename_all = "camelCase")]
pub enum Cmd {
  ExternalCommand { payload: ServerCmd },
  LocalCommand { payload: LocalCmd },
}
#[derive(Deserialize)]
#[serde(tag = "cmd", rename_all = "camelCase")]
pub enum ServerCmd {
  GetStatus,
  SetLoopback { value: bool },
  SetShouldRemoveNoise { value: bool },
  SetMicrophone { value: i32 },
  GetMicrophones,
}
#[derive(Deserialize)]
#[serde(tag = "cmd", rename_all = "camelCase")]
pub enum LocalCmd {
  Log { msg: String, level: i32 },
  Exit,
}
// #[derive(Deserialize, Debug)]
// #[serde(tag = "resp", rename_all = "camelCase")]
// pub enum Resp {
//   Ack,
//   Status(bool),
//   Error(String),
//
