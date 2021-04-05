import React, { useState, useEffect } from 'react';
import ReactDOM from 'react-dom';

import { invoke, promisified } from 'tauri/api/tauri'
import { listen } from 'tauri/api/event'
import { open } from 'tauri/api/window';

import './main.scss';
import './device_selector.scss';
import './select.scss';
import './toggle.scss';

import logo from './img/logo.png';
import mic from './img/mic.svg';
import speaker from './img/speaker.svg';

const makeExternalCmd = (p) => {
    return {
	cmd: "externalCommand",
	payload: p
    };
};
const makeLocalCmd = (p) => {
    return {
	cmd: "localCommand",
	payload: p
    };
};
const DEBUG = 0;
const ERROR = 1;
const INFO = 2;
const TRACE = 3;
const WARN = 4;
const log = (msg, level) => {
    invoke(makeLocalCmd({cmd: "log", msg, level}));
}

const SelectWithImage = ({options, image, chosen, setChosen}) => {
    return <div className="select-with-image">
	       <img src={image} />
	       <div className="vert-line"/>
	       <label>
	       <select 
		   value={chosen}
		   onChange={(e) => setChosen(e.target.value)}
	       >
		   {options.map((c) => <option key={c} value={c}> {c} </option>)}
	       </select>
	       </label>
	   </div>;
}
const DeviceSelector = ({title, current, icon, devices, switchToDevice}) => {
    let map = {};
    let cur;
    for (let i =0; i < devices.length; i++) {
	map[devices[i].name] = devices[i].id;
	if (devices[i].id === current) {
	    cur = devices[i].name;
	}
    }
    const [device, setDevice] = useState();
    const [remove, setRemove] = useState(false);
    useEffect(() => {
	setDevice(cur);
    }, [current]);
    useEffect(() => {
	promisified(makeExternalCmd({cmd: "setShouldRemoveNoise", value: remove}))
	    .then((v) => log(`setShouldRemoveNoise response: "${JSON.stringify(v)}"`, TRACE))
	    .catch((v) => log(`setShouldRemoveNoise error: "${JSON.stringify(v)}"`, ERROR));
    }, [remove]);
    return <div className="device-selector">
	       { /* <h1> {title} </h1> */ }
	       <SelectWithImage options={devices.map((e) => e.name)}
				image={icon}
				chosen={device}
				setChosen={(v) => {
				    setDevice(v);
				    switchToDevice(map[v])
					.then((e) => log(`switchToDevice response: "${JSON.stringify(e)}"`, TRACE))
					.catch((e) => log(`switchToDevice error: "${JSON.stringify(e)}"`, ERROR));
				}}/>
	       <div className="remove-noise">
		   <label className="text"> Remove Noise </label>
		   <label className="switch">
		       <input type="checkbox" checked={remove} onChange={(e) => setRemove(!remove)}/>
		       <span className="slider round"></span>
		   </label>
	       </div>
	   </div>;
}

const App = () => {
    const [devices, setDevices] = useState([]);
    const [curDevice, setCurrentDevice] = useState();
    const [loopback, setLoopback] = useState(false);
    const [status, setStatus] = useState("Waiting");
    useEffect(() => {
	// This clear interval stuff is definetly broken. Need to fix it
	const cb = () => {
	    promisified(makeExternalCmd({cmd: "getStatus"}))
		.then((v) => {
		    if (v === true) {
			setStatus("Good");
		    } else if (status === "Good"){
			setStatus("Lost");
		    } else {
			setStatus("Failed");
		    }
		    log(`getStatus response: "${JSON.stringify(v)}"`, TRACE)
		})
		.catch((v) => log(`getStatus error: "${JSON.stringify(v)}"`, ERROR))
		.finally((v) => {
		    let time = 10000;
		    switch (status){
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
	}
	cb()
    }, [status]);
    useEffect(() => {
	promisified(makeExternalCmd({cmd: "getMicrophones"}))
	    .then((v) => {
		setDevices(v["list"]);
		setCurrentDevice(v["cur"]);
		log(`getMicrophones response: "${JSON.stringify(v)}"`, TRACE)
	    })
	    .catch((v) =>
		log(`getMicrophones error: "${JSON.stringify(v)}"`, ERROR));
    }, []);
    useEffect(() => {
	// TODO: Handle errors
	if (devices.length > 0) {
	    promisified(makeExternalCmd({cmd: "setLoopback", value: loopback}))
		.then((v) =>
		    log(`setLoopback response: "${JSON.stringify(v)}"`, TRACE))
		.catch((v) =>
		    log(`setLoopback error: "${JSON.stringify(v)}"`, ERROR));
	}
    }, [loopback, devices]);

    switch (status) {
    case "Good":
	return <div id="main-container">
		   <img id="logo" src={logo} />
		   <DeviceSelector title="Microphone" current={curDevice} icon={mic} devices={devices} switchToDevice={(v) => promisified(makeExternalCmd({cmd: "setMicrophone", value: v}))}/>
		   {/*<DeviceSelector title="Speakers" icon={speaker} devices={[{name:"Speakers - System Default", id:0}]} /> */}
		   <div id="bottom">
		   <p id="loopback" onClick={() => setLoopback(!loopback)}> {loopback ? "Stop" : "Mic Check"} </p>
		       <a onClick={() => open("https://magicmic.ai/feedback")} id="feedback"> Give us your feedback! </a>
		   </div>
	       </div>;
    case "Failed":
	return <div id="error-container">
		   <h1> Failed to create virtual microphone, please restart the app. If
		   that doesn't work, contact us </h1>
	       </div>;
    case "Lost":
	return <div id="error-container">
		   <h1> Lost connection to virtual microphone! Please restart
		   the app. If that doesn't work, contact us </h1>
	       </div>;
    case "Waiting":
	return <div id="error-container">
		   <h1> Loading... </h1>
	       </div>;
    }
}

ReactDOM.render(
    <App />,
    document.getElementById('root')
);
