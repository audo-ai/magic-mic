import { invoke } from "@tauri-apps/api/tauri";

import { makeExternalCmd, trace, error } from "./utils";

export const getRemoveNoise = (cb) => {
  invoke("getRemoveNoise")
    .then((res) => {
      cb(res);
    })
    .catch((v) => {
      error(`getRemoveNoise error: "${JSON.stringify(v)}"`);
    });
};

export const setShouldRemoveNoise = (remove) => {
  invoke("setShouldRemoveNoise", { value: remove })
    .then((res) => {
      trace(`setShouldRemoveNoise response: "${JSON.stringify(res)}"`);
    })
    .catch((v) => {
      error(`setShouldRemoveNoise error: "${JSON.stringify(v)}"`);
    });
};

export const getProcessors = (cb) => {
  invoke("getProcessors")
    .then((res) => {
      cb(res);
    })
    .catch((v) => {
      error(`getAudioProcessors error: "${JSON.stringify(v)}"`);
    });
};

export const setProcessor = (id) => {
  invoke("setProcessor", { value: id })
    .then((v) => {
      trace(`setProcessor response: "${JSON.stringify(v)}"`);
    })
    .catch((v) => error(`setProcessor error: "${JSON.stringify(v)}"`));
};

export const getLoopback = (cb) => {
  invoke("getLoopback")
    .then((res) => {
      cb(res);
    })
    .catch((v) => error(`getLoopback error: "${JSON.stringify(v)}"`));
};

export const getStatus = (cb) => {
  invoke("getStatus")
    .then((v) => {
      cb(v);
      trace(`getStatus response: "${JSON.stringify(v)}"`);
    })
    .catch((v) => error(`getStatus error: "${JSON.stringify(v)}"`));
};

export const getMicrophones = (cb) => {
  invoke("getMicrophones")
    .then((v) => {
      cb(v);
      trace(`getMicrophones response: "${JSON.stringify(v)}"`);
    })
    .catch((v) => error(`getMicrophones error: "${JSON.stringify(v)}"`));
};

export const setLoopback = (loopback) => {
  invoke("setLoopback", { value: loopback })
    .then((v) => trace(`setLoopback response: "${JSON.stringify(v)}"`))
    .catch((v) => error(`setLoopback error: "${JSON.stringify(v)}"`));
};

export const setMicrophone = (microphone) => {
  invoke("setMicrophone", { value: microphone })
    .then((v) => trace(`setMicrophone response: "${JSON.stringify(v)}"`))
    .catch((v) => error(`setMicrophone error: "${JSON.stringify(v)}"`));
};
