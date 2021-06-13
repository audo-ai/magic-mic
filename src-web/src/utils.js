import { invoke } from "@tauri-apps/api/tauri";

const DEBUG = 0;
const ERROR = 1;
const INFO = 2;
const TRACE = 3;
const WARN = 4;

const log = (msg, level) => {
  invoke('jsLog', { msg: msg, level: level });
};

export const debug = (msg) => log(msg, DEBUG);
export const error = (msg) => log(msg, ERROR);
export const info = (msg) => log(msg, INFO);
export const trace = (msg) => log(msg, TRACE);
export const warn = (msg) => log(msg, WARN);
