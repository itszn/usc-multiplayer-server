package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"github.com/ThreeDotsLabs/watermill"
)

var (
	logger watermill.LoggerAdapter
)

var (
	VERSION                 = "v0.17"
	SCOREBOARD_REFERSH_RATE = 0
	DEBUG_LEVEL             = 0
	SERVER_PASS             = ""
)

func main() {
	f_VERBOSE := flag.Bool("verbose", false, "Print verbose info")
	f_DEBUG := flag.Bool("debug", false, "Print debug info")
	f_SCOREBOARD_REFERSH_RATE := flag.Int("refresh", 500, "Number of milliseconds between score updates (0 for realtime)")

	var bind string
	flag.StringVar(&bind, "bind", "0.0.0.0:39079", "host:port to bind to")

	flag.StringVar(&SERVER_PASS, "password", "", "server password if required")

	flag.Parse()

	// Setup watermill logging
	logger_impl := log.New(os.Stderr, "[watermill] ", log.LstdFlags|log.Lmicroseconds|log.Lshortfile)

	if *f_VERBOSE {
		DEBUG_LEVEL = 3 // Verbose
		logger = &watermill.StdLoggerAdapter{ErrorLogger: logger_impl, InfoLogger: logger_impl, DebugLogger: logger_impl}
	} else if *f_DEBUG {
		DEBUG_LEVEL = 2 // Debug
		logger = &watermill.StdLoggerAdapter{ErrorLogger: logger_impl, InfoLogger: logger_impl}
	} else {
		DEBUG_LEVEL = 1 // Info
		logger = &watermill.StdLoggerAdapter{ErrorLogger: logger_impl}
	}
	SCOREBOARD_REFERSH_RATE = *f_SCOREBOARD_REFERSH_RATE

	fmt.Println("Starting server on", bind)
	server := New_server(bind)
	server.Start()
}
