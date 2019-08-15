package main

import (
	"github.com/ThreeDotsLabs/watermill"
)

var (
	logger = watermill.NewStdLogger(false, false)
)

func main() {
	server := New_server("0.0.0.0:39079")
	server.Start()
}
