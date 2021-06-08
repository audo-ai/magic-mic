import { promisified } from "tauri/api/tauri";

import { makeExternalCmd, trace, error } from "./utils";

export const getRemoveNoise = (cb) => {
  promisified(makeExternalCmd({ cmd: "getRemoveNoise" }))
    .then((res) => {
      cb(res);
    })
    .catch((v) => {
      error(`getRemoveNoise error: "${JSON.stringify(v)}"`);
    });
};

export const setShouldRemoveNoise = (remove) => {
  promisified(makeExternalCmd({ cmd: "setShouldRemoveNoise", value: remove }))
    .then((res) => {
      trace(`setShouldRemoveNoise response: "${JSON.stringify(res)}"`);
    })
    .catch((v) => {
      error(`setShouldRemoveNoise error: "${JSON.stringify(v)}"`);
    });
};

export const getProcessors = (cb) => {
  promisified(makeExternalCmd({ cmd: "getProcessors" }))
    .then((res) => {
      cb(res);
    })
    .catch((v) => {
      error(`getAudioProcessors error: "${JSON.stringify(v)}"`);
    });
};

export const setProcessor = (id) => {
  promisified(makeExternalCmd({ cmd: "setProcessor", value: id }))
    .then((v) => {
      trace(`setProcessor response: "${JSON.stringify(v)}"`);
    })
    .catch((v) => error(`setProcessor error: "${JSON.stringify(v)}"`));
};

export const getLoopback = (cb) => {
  promisified(makeExternalCmd({ cmd: "getLoopback" }))
    .then((res) => {
      cb(res);
    })
    .catch((v) => error(`getLoopback error: "${JSON.stringify(v)}"`));
};

export const getStatus = (cb) => {
  promisified(makeExternalCmd({ cmd: "getStatus" }))
    .then((v) => {
      cb(v);
      trace(`getStatus response: "${JSON.stringify(v)}"`);
    })
    .catch((v) => error(`getStatus error: "${JSON.stringify(v)}"`));
};

export const getMicrophones = (cb) => {
  promisified(makeExternalCmd({ cmd: "getMicrophones" }))
    .then((v) => {
      cb(v);
      trace(`getMicrophones response: "${JSON.stringify(v)}"`);
    })
    .catch((v) => error(`getMicrophones error: "${JSON.stringify(v)}"`));
};

export const setLoopback = (loopback) => {
  promisified(makeExternalCmd({ cmd: "setLoopback", value: loopback }))
    .then((v) => trace(`setLoopback response: "${JSON.stringify(v)}"`))
    .catch((v) => error(`setLoopback error: "${JSON.stringify(v)}"`));
};

export const setMicrophone = (microphone) => {
  promisified(makeExternalCmd({ cmd: "setMicrophone", value: microphone }))
    .then((v) => trace(`setMicrophone response: "${JSON.stringify(v)}"`))
    .catch((v) => error(`setMicrophone error: "${JSON.stringify(v)}"`));
};
