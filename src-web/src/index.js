import React, { useState, useEffect } from 'react';
import ReactDOM from 'react-dom';

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
const DeviceSelector = ({title, icon, devices}) => {
    const [device, setDevice] = useState();
    const [remove, setRemove] = useState(false);
    return <div className="device-selector">
	       <h1> {title} </h1>
	       <SelectWithImage options={devices} image={icon} chosen={device} setChosen={setDevice}/>
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
    return <div id="main-container">
	       <img id="logo" src={logo} />
	       <DeviceSelector title="Microphone" icon={mic} devices={["Microphone - System Default"]} />
	       <DeviceSelector title="Speakers" icon={speaker} devices={["Speakers - System Default"]} />
	       <p id="test"> Test Noise Cancellation </p>
	   </div>;
}

ReactDOM.render(
    <App />,
    document.getElementById('root')
);
