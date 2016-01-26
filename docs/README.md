# Building and "installing"

You will need either Qt4 (>= 4.8) or Qt5 (>= 5.2) to build MegaBot (there are no other
external build dependencies):

```
git clone https://github.com/ileonte/megabot.git
cd megabot
qmake
make
```

"Installing" the bot consists of creating a base directory (hence-forth refered to as `$PREFIX`)
with a fixed structure of subdirectories. The results of the compilation are placed inside the 
`build` directory as follows:

* `build/bin` - contains the bot binary (`megabot.bin`)
* `build/lib` - contains the bundled Lua library (more info about this library can be found 
in the `Lua Scripting` section below)
* `build/share/lib` - contains Lua packages built against the Lua bundled Lua library
* `build/share/scripts` - contains Lua scripts (this is where you will need to place your custom
scripts in order for the bot to pick them up).

You are free to move the contents of the `build` directory anywhere on the filesystem
**provided** you keep the structure listed above unchanged.

In adition to the files/directories created by the build process you will need to create the following
items inside your `$PREFIX` directory in order for the bot to function correctly:

* `etc/config.json` - this file contains the configuration parameters for the bot (see the `Configuration`)
section below for more details
* `var/log` - this directory will be used to store the log file created by the bot. If you do not create this
directory logging will be effectivelly disabled.

Here is an example of a fully-functional `$PREFIX` directory (subdirectories marked with `(*)` are created
by the build process and can be copied as-is from the `build` directory):

```
├── bin (*)
│   └── megabot.bin
├── etc
│   └── config.json
├── lib (*)
│   ├── liblua.so -> liblua.so.5.0.0
│   ├── liblua.so.5 -> liblua.so.5.0.0
│   ├── liblua.so.5.0 -> liblua.so.5.0.0
│   └── liblua.so.5.0.0
├── share (*)
│   ├── lib
│   │   └── lua
│   │       ├── feedparser
│   │       │   ├── dateparser.lua
│   │       │   ├── url.lua
│   │       │   └── XMLElement.lua
│   │       ├── feedparser.lua
│   │       ├── JSON.lua
│   │       ├── liblsqlite3.so -> liblsqlite3.so.1.0.0
│   │       ├── liblsqlite3.so.1 -> liblsqlite3.so.1.0.0
│   │       ├── liblsqlite3.so.1.0 -> liblsqlite3.so.1.0.0
│   │       ├── liblsqlite3.so.1.0.0
│   │       ├── liblxp.so -> liblxp.so.1.0.0
│   │       ├── liblxp.so.1 -> liblxp.so.1.0.0
│   │       ├── liblxp.so.1.0 -> liblxp.so.1.0.0
│   │       ├── liblxp.so.1.0.0
│   │       ├── lxp
│   │       │   └── lom.lua
│   │       └── mbutils.lua
│   └── scripts
│       ├── at11.lua
│       ├── blackjack.lua
│       ├── cheese.lua
│       ├── feedwatch.lua
│       ├── roll.lua
│       ├── test.lua
│       ├── websearch.lua
│       └── xkcd.lua
└── var
    └── log
        └── megabot.log
```
