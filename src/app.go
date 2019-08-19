package main

import (
	"flag"

	"github.com/ThreeDotsLabs/watermill"
)

var (
	logger = watermill.NewStdLogger(false, false)
)

var (
	VERSION                 = "v0.12"
	SCOREBOARD_REFERSH_RATE = 0
	DEBUG_LEVEL             = 0
)

func main() {
	f_VERBOSE := flag.Bool("verbose", false, "Print verbose info")
	f_SCOREBOARD_REFERSH_RATE := flag.Int("refresh", 500, "Number of milliseconds between score updates (0 for realtime)")

	var bind string
	flag.StringVar(&bind, "bind", "0.0.0.0:39079", "host:port to bind to")

	flag.Parse()

	if *f_VERBOSE {
		DEBUG_LEVEL = 3 // Verbose
	} else {
		DEBUG_LEVEL = 1 // Info
	}
	SCOREBOARD_REFERSH_RATE = *f_SCOREBOARD_REFERSH_RATE

	server := New_server(bind)
	server.Start()
}
