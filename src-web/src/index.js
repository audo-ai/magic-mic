import React, { useState, useEffect, useRef } from "react";
import ReactDOM from "react-dom";

import { invoke, promisified } from "tauri/api/tauri";
import { open } from "tauri/api/window";

import "./main.scss";
import "./device_selector.scss";
import "./select.scss";
import "./toggle.scss";

import logo from "./img/logo.png";
import mic from "./img/mic.svg";
import speaker from "./img/speaker.svg";

const makeExternalCmd = (p) => {
  return {
    cmd: "externalCommand",
    payload: p,
  };
};
const makeLocalCmd = (p) => {
  return {
    cmd: "localCommand",
    payload: p,
  };
};
const DEBUG = 0;
const ERROR = 1;
const INFO = 2;
const TRACE = 3;
const WARN = 4;
const log = (msg, level) => {
  //invoke(makeLocalCmd({ cmd: "log", msg, level }));
};

const SelectWithImage = ({ options, image, chosen, setChosen }) => {
  return (
    <div className="select-with-image">
      <img src={image} />
      <div className="vert-line" />
      <label>
        <select value={chosen} onChange={(e) => setChosen(e.target.value)}>
          {options.map((c) => (
            <option key={c} value={c}>
              {" "}
              {c}{" "}
            </option>
          ))}
        </select>
      </label>
    </div>
  );
};
const DeviceSelector = ({ title, current, icon, devices, switchToDevice }) => {
  let map = {};
  let cur;
  for (let i = 0; i < devices.length; i++) {
    map[devices[i].name] = devices[i].id;
    if (devices[i].id === current) {
      cur = devices[i].name;
    }
  }
  const [device, setDevice] = useState();
  const [remove, setRemove] = useState(null);
  useEffect(() => {
    let cb = () => {
      promisified(makeExternalCmd({ cmd: "getRemoveNoise" }))
        .then((res) => {
          setRemove(res);
        })
        .catch((v) =>
          log(`getRemoveNoise error: "${JSON.stringify(v)}"`, ERROR)
        );
    };
    cb();
    let interval = setInterval(cb, 2000);
    return () => clearInterval(interval);
  }, [setRemove]);
  useEffect(() => {
    setDevice(cur);
  }, [current]);
  useEffect(() => {
    if (remove === null) {
      log("remove is null", TRACE);
      return;
    }
    promisified(makeExternalCmd({ cmd: "setShouldRemoveNoise", value: remove }))
      .then((v) =>
        log(`setShouldRemoveNoise response: "${JSON.stringify(v)}"`, TRACE)
      )
      .catch((v) =>
        log(`setShouldRemoveNoise error: "${JSON.stringify(v)}"`, ERROR)
      );
  }, [remove]);
  return (
    <div className="device-selector">
      {/* <h1> {title} </h1> */}
      <SelectWithImage
        options={devices.map((e) => e.name)}
        image={icon}
        chosen={device}
        setChosen={(v) => {
          setDevice(v);
          switchToDevice(map[v])
            .then((e) =>
              log(`switchToDevice response: "${JSON.stringify(e)}"`, TRACE)
            )
            .catch((e) =>
              log(`switchToDevice error: "${JSON.stringify(e)}"`, ERROR)
            );
        }}
      />
      <div className="remove-noise">
        <label className="text"> Remove Noise </label>
        <label className="switch">
          <input
            type="checkbox"
            checked={remove}
            onChange={(e) => setRemove(!remove)}
          />
          <span className="slider round"></span>
        </label>
      </div>
    </div>
  );
};
const Options = () => {
  const [isOpen, setIsOpen] = useState(false);
  const [procs, setProcs] = useState([]);
  const [curProc, setCurProc] = useState();
  const boxRef = useRef(null);
  const optionsRef = useRef(null);

  useEffect(() => {
    let cb = () => {
      promisified(makeExternalCmd({ cmd: "getProcessors" }))
        .then((v) => {
          setProcs(v["list"]);
          setCurProc(v["cur"]);
          log(`getAudioProcessors response: "${JSON.stringify(v)}"`, TRACE);
        })
        .catch((v) =>
          log(`getAudioProcessors error: "${JSON.stringify(v)}"`, ERROR)
        );
    };
    cb();
    let int = setInterval(cb, 5000);
    return () => clearInterval(int);
  }, []);
  const setProcessor = (id) => {
    if (id !== curProc) {
      promisified(makeExternalCmd({ cmd: "setProcessor", value: id }))
        .then((v) => {
          log(`setProcessor response: "${JSON.stringify(v)}"`, TRACE);
        })
        .catch((v) => log(`setProcessor error: "${JSON.stringify(v)}"`, ERROR));
      setCurProc(id);
    }
  };
  useEffect(() => {
    const docClick = (e) => {
      if (
        isOpen &&
        boxRef &&
        boxRef.current &&
        !boxRef.current.contains(e.target) &&
        optionsRef &&
        optionsRef.current &&
        !optionsRef.current.contains(e.target)
      ) {
        setIsOpen(false);
      }
    };
    document.addEventListener("mousedown", docClick);
    return () => {
      document.removeEventListener("mousedown", docClick);
    };
  }, [boxRef, isOpen, setIsOpen]);

  return (
    <>
      <div
        ref={optionsRef}
        id="options-toggle"
        className={isOpen ? "open" : "closed"}
        onClick={(e) => setIsOpen(!isOpen)}
      >
        {" "}
      </div>
      <div id="options" style={isOpen ? {} : { display: "none" }} ref={boxRef}>
        <div className="clickable" onClick={() => open("https://magicmic.ai/")}>
          {" "}
          About
        </div>
        <div
          className="clickable"
          onClick={() => open("https://github.com/audo-ai/magic-mic")}
        >
          Github
        </div>
        <div
          className="clickable"
          onClick={() =>
            open("https://github.com/audo-ai/magic-mic/discussions")
          }
        >
          Discussions
        </div>
        <hr />
        <div id="model-select-prompt">Choose a noise cancellation model</div>
        <ul>
          {procs.map((p, i) => (
            <li
              onClick={() => setProcessor(i)}
              className={i === curProc ? "selected" : ""}
              key={p.name}
            >
              {p.name}
            </li>
          ))}
        </ul>
        <div id="reselect-notice">
          (You may need to reselect Magic Mic as your microphone)
        </div>
      </div>
    </>
  );
};
const App = () => {
  const [devices, setDevices] = useState([]);
  const [curDevice, setCurrentDevice] = useState();
  const [loopback, setLoopback] = useState(null);
  const [status, setStatus] = useState("Waiting");
  useEffect(() => {
    let cb = () => {
      promisified(makeExternalCmd({ cmd: "getLoopback" }))
        .then((res) => {
          setLoopback(res);
        })
        .catch((v) => log(`getLoopback error: "${JSON.stringify(v)}"`, ERROR));
    };
    cb();
    let interval = setInterval(cb, 2000);
    return () => clearInterval(interval);
  }, [setLoopback]);
  useEffect(() => {
    // This clear interval stuff is definetly broken. Need to fix it
    const cb = () => {
      promisified(makeExternalCmd({ cmd: "getStatus" }))
        .then((v) => {
          if (v === true) {
            setStatus("Good");
          } else if (status === "Good") {
            setStatus("Lost");
          } else {
            setStatus("Failed");
          }
          log(`getStatus response: "${JSON.stringify(v)}"`, TRACE);
        })
        .catch((v) => log(`getStatus error: "${JSON.stringify(v)}"`, ERROR))
        .finally((v) => {
          let time = 10000;
          switch (status) {
            case "Good":
              time = 10000;
              break;
            case "Waiting":
              time = 500;
            case "Lost":
            case "Failed":
              return;
          }
          setTimeout(cb, time);
        });
    };
    cb();
  }, [status]);
  useEffect(() => {
    let cb = () => {
      promisified(makeExternalCmd({ cmd: "getMicrophones" }))
        .then((v) => {
          setDevices(v["list"]);
          setCurrentDevice(v["cur"]);
          log(`getMicrophones response: "${JSON.stringify(v)}"`, TRACE);
        })
        .catch((v) =>
          log(`getMicrophones error: "${JSON.stringify(v)}"`, ERROR)
        );
    };
    cb();
    let int = setInterval(cb, 5000);
    return () => clearInterval(int);
  }, []);
  useEffect(() => {
    // TODO: Handle errors
    if (loopback === null) {
      return;
    }
    if (devices.length > 0) {
      promisified(makeExternalCmd({ cmd: "setLoopback", value: loopback }))
        .then((v) => log(`setLoopback response: "${JSON.stringify(v)}"`, TRACE))
        .catch((v) => log(`setLoopback error: "${JSON.stringify(v)}"`, ERROR));
    }
  }, [loopback, devices]);
  useEffect(() => {
    // TODO: Handle errors
    if (loopback === null) {
      return;
    }
    if (devices.length > 0) {
      promisified(makeExternalCmd({ cmd: "setLoopback", value: loopback }))
        .then((v) => log(`setLoopback response: "${JSON.stringify(v)}"`, TRACE))
        .catch((v) => log(`setLoopback error: "${JSON.stringify(v)}"`, ERROR));
    }
  }, [loopback, devices]);

  switch (status) {
    case "Good":
      return (
        <div id="main-container">
          <Options />
          <img id="logo" src={logo} />
          <DeviceSelector
            title="Microphone"
            current={curDevice}
            icon={mic}
            devices={devices}
            switchToDevice={(v) =>
              promisified(
                makeExternalCmd({
                  cmd: "setMicrophone",
                  value: v,
                })
              )
            }
          />
          {/*<DeviceSelector title="Speakers" icon={speaker} devices={[{name:"Speakers - System Default", id:0}]} /> */}
          <div id="bottom">
            <p id="loopback" onClick={() => setLoopback(!loopback)}>
              {" "}
              {loopback ? "Stop" : "Mic Check"}{" "}
            </p>
          </div>
        </div>
      );
    case "Failed":
      return (
        <div id="error-container">
          <h1>
            {" "}
            Failed to create virtual microphone, please restart the app. If that
            doesn\'t work, contact us{" "}
          </h1>
        </div>
      );
    case "Lost":
      return (
        <div id="error-container">
          <h1>
            {" "}
            Lost connection to virtual microphone! Please restart the app. If
            that doesn\'t work, contact us{" "}
          </h1>
        </div>
      );
    case "Waiting":
      return (
        <div id="error-container">
          <h1> Loading... </h1>
        </div>
      );
  }
};

ReactDOM.render(<App />, document.getElementById("root"));
