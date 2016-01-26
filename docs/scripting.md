## General

MegaBot features an extensible scripting framework designed to work with multiple
scripting languages. Lua is the first supported language with more to follow.

## Lua scripting

By default MegaBot will load the bundled Lua library (`$PREFIX/lib/liblua.so`) in
order to run scripts. If you would prefer to use some other Lua library you can
do so by using the `luaLib` configuration option at the script level:

```js
{
	"servers": {
		"myserver": {

			....

			"rooms": {
				"roomname": {
					
					...

					"scripts": {
						"script.lua": {
							"luaLib": "/usr/lib/liblua.so"
						}

						...
					}
				}
			}
		}
	}
}
```

### Required functions

The following functions must be defined by all Lua scripts. Failure to do so will
cause the bot to reject the script.

#### handle_room_config

```lua
function handle_room_config( config )
```

This function is called whenever room configuration changes. In particular this is
called immediatelly after the bot has joined a room. The 'config' parameter is a
Lua table with the following fields:

- `roomName`: the name of the chatroom this script is running in
- `roomJid`: the full JID of the chatroom this script is running in
(roomName@conferencehost)
- `nickName`: the nickname the bot is using in this chatroom
- `topic`: the topic (subject) of the chatroom

#### handle_room_presence

```lua
function handle_room_presence( presence )
```

This function is called whenever a chat room participant's presence changes. This is also
called for presence changes for the bot itself. The 'presence' parameter is a Lua table
with the following fields:

- `who`: the full room JID for the entity this event refers to (room@conferencehost/nickName)
- `nickName`: the nickname of the entity this event refers to
- `presence`: the current presence of the entity this event refers to. A value of
`PRES_Available` signals that the entity has just joined the chat room. A
value of `PRES_Unavailable` signals that the entity has just left the chat room.
- `statusType`: the current status type. Will be one of:
     * `STAT_Offline`
     * `STAT_Online`
     * `STAT_Away`
     * `STAT_XA` (extended away)
     * `STAT_DND`
     * `STAT_Chat`
     * `STAT_Invisible`
- `statusText`: the status text set by the user
- `statusPriority`: the priority of the presence

#### handle_room_message

```lua
function handle_room_message( message )
```

This function is called whenever a room message is received. It will NOT be called for
messages that have been sent by the bot itself (to avoid loops). The 'message' parameter
is a Lua table with the following fields:

- `nickName`: the nickname of the sender of the message
- `from`: the full room JID of the sender of the message (room@conferencehost/nickName)
- `body`: the message body

### Lua scripting API

The following functions are available for calling from Lua code.

#### send_room_message
```lua
(noreturn) send_room_message( message )
```
Sends 'message' to the chat room.

#### get_participants
```lua
table get_participants()
```
Returns a list of all participants currently present in the chatroom.

#### network_request
```lua
(noreturn) network_request( [name,] url )
```
Starts an asynchronous network request (GET) on `url`. The optional parameter `name` can
be used by scripts as an identifier for the request. If `name` is not specified a random
name is assigned to the request. If this function is used you MUST define the special
reply handling function `handle_network_reply` (see below for details). Failure to do
so will raise an error and will lead to the script being terminated.

```lua
function handle_network_reply( reply )
```
This function is called whenever an asynchronous network reply finished, either normally
or due to an error. The `reply` parameter is a Lua table with the following fields:

- `status`: will be set to 'true' (boolean) if the request was successfully completed,
or to 'false' if an error occured.
- `data`: if `status` is true then this field contains the data that was fetched from
the url. If `status` is false then this field contains an error message describing the error 
that occured
- `name`: specifies the name of the asynchronous request that generated the event
- `url`: specifies the URL of the request

**NOTE:** URLs must be fully qualified (eg `protocol://[username[:password]@]some.host[:port][/some/path][?params]`).
The protocols that are supported depend on the capabilities of the underlying Qt
library. In general it is safe to assume that at least HTTP, HTTPS and FTP are
supported.

**NOTE:** a limited number of network requests can be started at the same time. The default
limit is 10. You can change this value at compile-time by modifying the value of
`MB_MAX_NETREQ_COUNT` in `megabot/cscriptrunner.h`


#### set_timer
```lua
(noreturn) set_timer( [name,] timeout )
```
Create a new timer object that will activate after 'timeout' miliseconds have elapsed
by calling the special handling function `handle_timer_event` (see below for details).
The optional parameter `name` can be used by scripts as an identifier for the timer.
If `name` is not specified a random name is assigned to the timer.

```lua
function handler_timer_event( timer_name )
```
This function is called whenever a timer ticks. `timer_name` is a string that
represents the timer object that generated this event.

**NOTE:** timers are 'single-shot'. This means that once a timer ticks it becomes inactive.
Scripts can re-arm the timer by calling `set_timer()` from `handle_timer_event()`.

**NOTE:** a limited number of timers can be started at the same time. The default limit is
set to 10. You can change this value at compile-time by modifying the value of
`MB_MAX_TIMER_COUNT` in `megabot/cscriptrunner.h`

#### tokenize
```lua
table tokenize( string )
```
Splits the strings into 'tokens' separated by white-space and returns them as a list.
This function supports quoted sub-strings and escape sequences. For example, the string

```lua
"This 'is a \'tokenize\'' example"
```
will return the following table:

```lua
[ "This", "is a 'tokenize'", "example" ]
```
