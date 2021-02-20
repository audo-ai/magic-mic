# IPC Protocol

I'll describe the electron/gui-native virtual mic protocol here. It's [JSON-RPC
2.0](https://www.jsonrpc.org/specification), so I'll be describing method
name's, parameters, results, errors, and notifications.

The protocol is just that messages are passed null char delimited. Some messages
are requests and expect a message response, but there is no specific
distinction. Both parties should expect any message at any time.

## Methods
### `getStatus`
Get's status of native engine; checks if it is ready to do everything else
(ie. its initialized whatever it needs to initialize)
#### Request
*Parameters*: None
```json
{
	"jsonrpc": "2.0",
	"method": "getStatus",
	"id": "exampleid"
}
```
#### Response
*Results*: bool

true=success

false=initializing
```json
{
	"jsonrpc": "2.0",
	"result": true,
	"id": "exampleid"
}
```
#### Errors
##### Miscelaneous error
*Code*: 0

*Fatal:* Yes
```json
{
	"jsonrpc": "2.0",
	"error": {
		"code": 0,
		"error": description of what happened,
	},
	"id": "exampleid"
}
```
### `getMicrophones`
Get's available microphones to listen on
#### Request
*Parameters*: None
```json
{
	"jsonrpc": "2.0",
	"method": "getMicrophones",
	"id": "exampleid"
}
```
#### Response
*Results*: Array of {"id": number, "name": string}
```json
{
	"jsonrpc": "2.0",
	"result": true,
	"id": "exampleid"
}
```
### `setMicrophone`
Sets the microphone to denoise
#### Request
*Parameters*: number, id recieved from getMicrophones
```json
{
	"jsonrpc": "2.0",
	"method": "setMicrophones",
	"parms": number,
	"id": "exampleid"
}
```
#### Response
*Results*: null
```json
{
	"jsonrpc": "2.0",
	"result": true,
	"id": "exampleid"
}
```
#### Errors
##### Failed to set microphone
*Code*: 200

*Fatal:* No
```json
{
	"jsonrpc": "2.0",
	"error": {
		"code": 200,
		"error": "Failed to set microphone",
	},
	"id": "exampleid"
}
```
### `setRemoveNoise`
starts/stops removing noise
#### Request
*Parameters*: bool
```json
{
	"jsonrpc": "2.0",
	"method": "setRemoveNoise",
	"id": "exampleid"
}
```
#### Response
*Results*: null
```json
{
	"jsonrpc": "2.0",
	"id": "exampleid"
}
```
