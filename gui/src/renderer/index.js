const rpc = require('electron').remote.require("./rpc");
const app = require('electron').remote.app;
const process = require('electron').remote.process;

import React, { useState, useEffect } from 'react';
import ReactDOM from 'react-dom';

import '../static/main.scss';
import '../static/device_selector.scss';
import '../static/select.scss';
import '../static/toggle.scss';

import logo from '../static/logo.png';
import mic from '../static/mic.svg';
import speaker from '../static/speaker.svg';

const SelectWithImage = ({options, image, chosen, setChosen}) => {
    return <div className="select-with-image">
	       <img src={image} />
	       <div class="vert-line"/>
	       <select 
		   value={chosen}
		   onChange={(e) => setChose(e.target.value)}
	       >
		   {options.map((c) => <option key={c} value={c}> {c} </option>)}
	       </select>
	   </div>;
}
const DeviceSelector = ({subP, title, icon, devices}) => {
    const [device, setDevice] = useState();
    const [remove, setRemove] = useState(false);
    useEffect(() => {
	if (subP) {
	    console.log("reove: " + remove);
	    rpc.setRemoveNoise(subP, remove);
	}
    }, [subP, remove]);
    return <div className="device-selector">
	       <h1> {title} </h1>
	       <SelectWithImage options={devices} image={icon} chosen={device} setChosen={setDevice}/>
	       <div className="remove-noise">
		   <label class="text"> Remove Noise </label>
		   <label class="switch">
		       <input type="checkbox" checked={remove} onChange={(e) => setRemove(!remove)}/>
		       <span class="slider round"></span>
		   </label>
	       </div>
	   </div>;
}

const App = () => {
    const [subP, setSubP] = useState();
    useEffect(() => {
	let s = rpc.init();
	setSubP(s);
	s.stdout.on('data', console.log);
	app.on('before-quit', () => {
	    s.kill('SIGINT');
	});
    }, []);
    return <div id="main-container">
	       <img id="logo" src={logo} />
	       <DeviceSelector subP={subP} title="Microphone" icon={mic} devices={["Microphone - System Default"]} />
	       <DeviceSelector subP={subP} title="Speakers" icon={speaker} devices={["Speakers - System Default"]} />
	       <p id="test"> Test Noise Cancellation </p>
	   </div>;
}

ReactDOM.render(
    <App />,
    document.getElementById('app')
);
