# usc-multiplayer-server

This is a multiplayer server for [unnamed-sdvx-clone](https://github.com/Drewol/unnamed-sdvx-clone)

## Docs 
[View Protocol Docs](https://itszn.github.io/usc-multiplayer-server/server.html)

## Getting the server

Check out the [release page](https://github.com/itszn/usc-multiplayer-server/releases) for some builds.

Otherwise see below of building

## Running

```
Usage of ./usc_multiplayer:
  -bind string
    	host:port to bind to (default "0.0.0.0:39079")
  -debug
    	Print debug info
  -password string
    	server password if required
  -refresh int
    	Number of milliseconds between score updates (0 for realtime) (default 500)
  -verbose
    	Print verbose info
```

`-refresh` controls how fast the scoreboard is updated. If you are playing on lan, try out 0 for basically realtime

## Building
[Install go 1.12+](https://golang.org/doc/install)

```
cd src
# Install required go dependencies
go get

# Build the server
go build
```
