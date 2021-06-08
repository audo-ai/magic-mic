import React, { useState, useEffect, useRef } from "react";
import ReactDOM from "react-dom";

import { open } from "tauri/api/window";

import {
  getStatus,
  getMicrophones,
  getRemoveNoise,
  setShouldRemoveNoise,
  getProcessors,
  setProcessor,
  getLoopback,
  setLoopback,
  setMicrophone,
} from "./cmd";
import { trace } from "./utils";

import "./main.scss";
import "./device_selector.scss";
import "./select.scss";
import "./toggle.scss";

import logo from "./img/logo.png";
import mic from "./img/mic.svg";

const SelectWithImage = ({ options, image, chosen, setChosen }) => {
  return (
    <div className="select-with-image">
      <img src={image} alt="decorative for select" />
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
      getRemoveNoise(setRemove);
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
      trace("remove is null");
      return;
    }
    setShouldRemoveNoise(remove);
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
          switchToDevice(map[v]);
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
      getProcessors((v) => {
        setProcs(v["list"]);
        setCurProc(v["cur"]);
        trace(`getAudioProcessors response: "${JSON.stringify(v)}"`);
      });
    };
    cb();
    let int = setInterval(cb, 5000);
    return () => clearInterval(int);
  }, []);
  const changeProcessor = (id) => {
    if (id !== curProc) {
      setProcessor(id);
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
              onClick={() => changeProcessor(i)}
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
  const [loopback, setShouldLoopback] = useState(null);
  const [status, setStatus] = useState("Waiting");
  useEffect(() => {
    let cb = () => {
      getLoopback(setShouldLoopback);
    };
    cb();
    let interval = setInterval(cb, 2000);
    return () => clearInterval(interval);
  }, [setShouldLoopback]);
  useEffect(() => {
    // This clear interval stuff is definetly broken. Need to fix it
    const cb = () => {
      getStatus((v) => {
        if (v === true) {
          setStatus("Good");
        } else if (status === "Good") {
          setStatus("Lost");
        } else {
          setStatus("Failed");
        }
        trace(`getStatus response: "${JSON.stringify(v)}"`);
      });
      setTimeout(cb, 1000);
    };
    cb();
  }, [status]);
  useEffect(() => {
    let cb = () => {
      getMicrophones((v) => {
        setDevices(v["list"]);
        setCurrentDevice(v["cur"]);
      });
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
      setLoopback(loopback);
    }
  }, [loopback, devices]);
  useEffect(() => {
    // TODO: Handle errors
    if (loopback === null) {
      return;
    }
    if (devices.length > 0) {
      setLoopback(loopback);
    }
  }, [loopback, devices]);

  switch (status) {
    case "Good":
      return (
        <div id="main-container">
          <Options />
          <img id="logo" src={logo} alt="logo" />
          <DeviceSelector
            title="Microphone"
            current={curDevice}
            icon={mic}
            devices={devices}
            switchToDevice={(v) => setMicrophone(v)}
          />
          {/*<DeviceSelector title="Speakers" icon={speaker} devices={[{name:"Speakers - System Default", id:0}]} /> */}
          <div id="bottom">
            <p id="loopback" onClick={() => setShouldLoopback(!loopback)}>
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
