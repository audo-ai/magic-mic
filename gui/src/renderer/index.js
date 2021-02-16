import React, { useState } from 'react';
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
	       <select 
		   value={chosen}
		   onChange={(e) => setChose(e.target.value)}
	       >
		   {options.map((c) => <option key={c} value={c}> {c} </option>)}
	       </select>
	   </div>;
}
const DeviceSelector = ({title, icon, devices}) => {
    const [device, setDevice] = useState();
    return <div className="device-selector">
	       <h1> {title} </h1>
	       <SelectWithImage options={devices} image={icon} chosen={device} setChosen={setDevice}/>
	       <div className="remove-noise">
		   <label class="text"> Remove Noise </label>
		   <label class="switch">
		       <input type="checkbox" />
		       <span class="slider round"></span>
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
    document.getElementById('app')
);
