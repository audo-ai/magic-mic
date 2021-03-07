import React, { useState, useEffect } from 'react';
import ReactDOM from 'react-dom';

import { invoke, promisified } from 'tauri/api/tauri'
import { listen } from 'tauri/api/event'

import './main.scss';
import './device_selector.scss';
import './select.scss';
import './toggle.scss';

import logo from './img/logo.png';
import mic from './img/mic.svg';
import speaker from './img/speaker.svg';

const DEBUG = 0;
const ERROR = 1;
const INFO = 2;
const TRACE = 3;
const WARN = 4;
const log = (msg, level) => {
    invoke({cmd: "log", msg, level});
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
const DeviceSelector = ({title, icon, devices, switchToDevice}) => {
    let map = [];
    for (let i =0; i < devices.length; i++) {
	map[devices[i].name] = devices[i].id;
    }
    const [device, setDevice] = useState();
    const [remove, setRemove] = useState(false);
    useEffect(() => {
	promisified({cmd: "setShouldRemoveNoise", value: remove})
	    .then((v) => log(`setShouldRemoveNoise response: "${JSON.stringify(v)}"`, TRACE))
	    .catch((v) => log(`setShouldRemoveNoise error: "${JSON.stringify(v)}"`, ERROR));
    }, [remove]);
    return <div className="device-selector">
	       <h1> {title} </h1>
	       <SelectWithImage options={devices.map((e) => e.name)}
				image={icon}
				chosen={device}
				setChosen={(v) => {
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
    const [loopback, setLoopback] = useState(false);
    useEffect(() => {
	// TODO: Handle errors
	setInterval(() => {
	    promisified({cmd: "getStatus"})
		.then((v) => log(`getStatus response: "${JSON.stringify(v)}"`, TRACE))
		.catch((v) => log(`getStatus error: "${JSON.stringify(v)}"`, ERROR))
	}, 10000);
    }, []);
    useEffect(() => {
	promisified({cmd: "getMicrophones"})
	    .then((v) => {
		setDevices(v);
		log(`getMicrophones response: "${JSON.stringify(v)}"`, TRACE)
	    })
	    .catch((v) =>
		log(`getMicrophones error: "${JSON.stringify(v)}"`, ERROR));
    }, []);
    useEffect(() => {
	// TODO: Handle errors
	if (devices.length > 0) {
	promisified({cmd: "setLoopback", value: loopback})
		.then((v) =>
		    log(`setLoopback response: "${JSON.stringify(v)}"`, TRACE))
		.catch((v) =>
		    log(`setLoopback error: "${JSON.stringify(v)}"`, ERROR));
	}
    }, [loopback, devices]);

    return <div id="main-container">
	       <img id="logo" src={logo} />
	       <DeviceSelector title="Microphone" icon={mic} devices={devices} switchToDevice={(v) => promisified({cmd: "setMicrophone", value: v})}/>
	       <DeviceSelector title="Speakers" icon={speaker} devices={[{name:"Speakers - System Default", id:0}]} />
	       <p id="loopback" onClick={() => setLoopback(!loopback)}> {loopback ? "Stop listening to yourself" : "Listen in on yourself"} </p>
	   </div>;
}

ReactDOM.render(
    <App />,
    document.getElementById('root')
);
