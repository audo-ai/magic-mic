/*
  TODO:
  - either don't use hashes or make hashes better (or accept that it doesn't
    have to be hugely scalable)
*/
import { shell } from 'electron';
import React, { useState } from 'react';
import ReactDOM from 'react-dom';

import '../static/main.css';
import '../static/slider.css';

// TODO load these inline so we can use css to change animation state
import anim_logo from '../static/audo_logo_animated.svg';
import logo from '../static/audo_logo_outline.svg';

const ExternalLink = (props) => {
    const handleOnClick = (e) => {
	e.preventDefault();
	shell.openExternal(props.href);
    };
    return <a id="qs"
	      onAuxClick={handleOnClick}
	      onClick={handleOnClick}
	      href={props.href}>
	       {props.children}
	   </a>;
}

const Questions = () => {
    return <div id="questions" >
	       <p> Platform Dependent info on how things work as well as controls</p>
	       <button onClick={() => window.location.hash = ""}> Back </button>
	   </div>;
};

const App = () => {
    const [checked, setChecked] = useState("");
    let img = logo;
    if (checked) {
	img = anim_logo;
    }
    return <>
	       <div id="app-top">
		   <h1> <em> <b>Magic Mic </b> </em></h1>
		   <label id="main-toggle" class="switch">
		       <input type="checkbox" onChange={() => setChecked(!checked)} checked={checked ? "yes" : ""}/>
		       <span className="slider round"></span>
		   </label>
		   <img id="logo" src={img} />
		   <div id="bottom-notes">
		       <p> A magical denoiser for your microphone
			   {/* Todo make this nicer */}
			   <span onClick={() => window.location.hash = "questions"} id="qmark">
			       <p><b>?</b></p>
			   </span>
		       </p>
		       <p> Made with artificial intelligence by the folks at <ExternalLink href="https://audo.ai"> Audo ai </ExternalLink> </p>
		   </div>
	       </div>
	       <Questions />
	   </>;
}

ReactDOM.render(
	<App />,
    document.getElementById('app')
);
