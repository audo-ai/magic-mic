const { spawn } = require('child_process');
import { app } from 'electron'

export const init = () => {
    let subP = spawn('/home/gabe/code/audo/project-x/native/build/server', {stdio: ['pipe', 'pipe', 'inherit']})
    return subP;
}
export const setRemoveNoise = (subP, bool) => {
    subP.stdin.write(JSON.stringify({
	"jsonrpc": "2.0",
	"method":"setRemoveNoise",
	"id": "a",
	"params": bool
    }));
    subP.stdin.write('\n');
}
