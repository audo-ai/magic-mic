# IPC Protocol

## Backstory
The app consists of two binraries: a front end tauri app (aka rust and react)
and a backend c++ daemon. The c++ daemon is responsible for actually creating
the virtual mic and running when the front end app closes. The two binaries
interact with [JSON-RPC 2.0](https://www.jsonrpc.org/specification) over a unix
socket (where only on linux right now). The "over the wire" protocol is just
json messages terminated with newlines.

## Methods
I'll describe the various methods the backend server supports.

### `getStatus`
Get's status of native engine; checks if it is ready to do everything else
(ie. it has initialized whatever it needs to initialize). Returns `True` if read
`False` otherwise.

### `getMicrophones`
Get's available microphones to listen on. Returns an object like
```json
{
	"list": [{
		"id": int,
		"name": string,
		}],
	"cur": string,
}
```
where list is the list of available listening devices with an integer id and a
display name. `cur` is the name of the currently active device.

### `setMicrophone`
Sets the microphone to use. Takes an integer parameter corresponding to the `id`
recieved from `getMicrophones`. Returns a boolean corresponding to whether the
operation succeeded.

### `setRemoveNoise`
Starts/stops removing noise. Takes a bool corresponding to whether or not to denoise.
No return value.

### `setLoopback`
starts/stops loopback. Takes a bool corresponding to whether or not to start the loopback.
