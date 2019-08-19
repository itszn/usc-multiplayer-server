package main

import (
	"github.com/ThreeDotsLabs/watermill"
)

var (
	logger = watermill.NewStdLogger(false, false)
)

var (
	VERSION                 = "v0.12"
	SCOREBOARD_REFERSH_RATE = 200
)

func main() {
	server := New_server("0.0.0.0:39079")
	server.Start()
}
