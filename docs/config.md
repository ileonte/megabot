## Configuration

Bot configuration is stored inside the `$PREFIX/etc/config.json` file. The format is pretty much
standard JSON with the notable exception that it supports comments:

```js
#
# This is a comment
#
{
	...
}
```

The file must contain a single global object which defines the configuration.

Example:

```js
{
	"servers": {
		"myserver": {
			"account": "megabot",
			"password": "SUPERSECRETPASSWORD",
			"domain": "example.com",
			"host": "xmpp.example.com",
			"conferenceHost": "conference.example.com",
			"resource": "MegaBot",

			"rooms": {
				"roomname": {
					"nickName": "MegaBot",
					"password": "",
					"scripts": {
						"script.lua": {}
					}
				}
			}
		}
	}
}
```
