import React, { useState, useEffect } from 'react';
import ReactDOM from 'react-dom';

import { promisified } from 'tauri/api/tauri'
import { listen } from 'tauri/api/event'

import './main.scss';
import './device_selector.scss';
import './select.scss';
import './toggle.scss';

import logo from './img/logo.png';
import mic from './img/mic.svg';
import speaker from './img/speaker.svg';

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
	    .then((v) => console.log(JSON.stringify(v)))
	    .catch((v) => console.log(JSON.stringify(v)));
    }, [remove]);
    return <div className="device-selector">
	       <h1> {title} </h1>
	       <SelectWithImage options={devices.map((e) => e.name)}
				image={icon}
				chosen={device}
				setChosen={(v) => {
				    switchToDevice(map[v])
					.then(console.log)
					.catch(console.error);
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
    useEffect(() => {
	setInterval(() => {
	    promisified({cmd: "getStatus"})
		.then((v) => console.log(JSON.stringify(v)))
		.catch((v) => console.log(JSON.stringify(v)));
	}, 10000);
    }, []);
    useEffect(() => {
	promisified({cmd: "getMicrophones"})
	    .then((v) => {
		setDevices(v);
		console.log(JSON.stringify(v))
	    })
	    .catch((v) => console.log(JSON.stringify(v)));
    }, []);

    return <div id="main-container">
	       <img id="logo" src={logo} />
	       <DeviceSelector title="Microphone" icon={mic} devices={devices} switchToDevice={(v) => promisified({cmd: "setMicrophone", value: v})}/>
	       <DeviceSelector title="Speakers" icon={speaker} devices={[{name:"Speakers - System Default", id:0}]} />
	       <p id="test"> Test Noise Cancellation </p>
	   </div>;
}

ReactDOM.render(
    <App />,
    document.getElementById('root')
);
