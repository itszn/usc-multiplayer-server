package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/ThreeDotsLabs/watermill"
	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/infrastructure/gochannel"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
	"github.com/ThreeDotsLabs/watermill/message/router/plugin"
	"github.com/google/uuid"
)

type Score struct {
	score uint32
	combo uint32
	clear uint32
}

type Score_point struct {
	score uint32
	time  uint32
}

// If a topic is in the set, no more messages will
// be processed for this user until Unblock is called
//var Blocking_topics map[string]void

type User struct {
	server *Server
	id     string
	name   string
	authed bool

	room  *Room
	score *Score

	score_list []Score_point

	ready       bool
	missing_map bool
	synced      bool

	playing     bool
	hard_mode   bool
	mirror_mode bool
	level       int

	conn   net.Conn
	reader *bufio.Reader
	writer *bufio.Writer

	pub_sub message.PubSub
	router  *message.Router

	mtx sync.RWMutex

	// Used to block new messages from being processed
	msg_block sync.Mutex
}

func New_user(conn net.Conn, server *Server) (*User, error) {
	user := &User{
		id:     uuid.New().String(),
		authed: false,

		server: server,
		room:   nil,

		score:       nil,
		ready:       false,
		missing_map: false,
		playing:     false,

		conn:   conn,
		reader: bufio.NewReader(conn),
		writer: bufio.NewWriter(conn),
	}

	if err := user.init(); err != nil {
		return nil, err
	}

	return user, nil
}

func (self *User) Unblock() {
	self.msg_block.Unlock()
}

func (self *User) Add_new_score(score uint32, time uint32) {
	self.mtx.Lock()
	defer self.mtx.Unlock()

	self.score_list = append(self.score_list, Score_point{
		score: score,
		time:  time,
	})
}

func (self *User) Get_last_score_time() uint32 {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	l := len(self.score_list)

	if l == 0 {
		return 0
	}

	return self.score_list[l-1].time
}

func (self *User) Get_score_at(time uint32) uint32 {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	for i := len(self.score_list) - 1; i >= 0; i-- {
		if self.score_list[i].time <= time {
			return self.score_list[i].score
		}
	}
	return 0
}

func (self *User) init() error {
	self.pub_sub = gochannel.NewGoChannel(gochannel.Config{}, logger)

	router, err := message.NewRouter(message.RouterConfig{}, logger)
	if err != nil {
		fmt.Println("Error ", err)
		return err
	}

	self.router = router

	// Add handler to shut down router
	router.AddPlugin(plugin.SignalsHandler)

	// Add router HandlerFunc -> HandlerFunc middleware
	router.AddMiddleware(
		// correlation ID will copy correlation id from consumed message metadata to produced messages
		middleware.CorrelationID,

		Error_middleware,

		Panic_middleware,

		Json_middleware,
	)

	self.add_routes()
	return nil
}

func (self *User) Start() {
	go self.read_loop()

	if err := self.router.Run(); err != nil {
		fmt.Println("Error ", err)
		self.destroy()
	}
}

func (self *User) destroy() {
	self.server.Remove_user(self)

	if self.room != nil {
		self.room.Remove_user(self)
	}

	self.conn.Close()
	self.router.Close()
}

func (self *User) read_loop() {
	// If this loop exits, clean up user
	defer self.destroy()

	// Sleep for a short bit to avoid race with the router
	time.Sleep(100 * time.Millisecond)

	// Use to catch ^C
	sigchan := make(chan os.Signal, 1)
	signal.Notify(sigchan, os.Interrupt)

	for {
		select {
		case <-sigchan:
			return
		default:
		}

		mode, err := self.reader.ReadByte()
		if err != nil {
			fmt.Println("Error reading: ", err.Error())
			return
		}

		// Newline delinitated JSON
		if mode == 1 {
			line, err := self.reader.ReadBytes('\n')
			if err != nil {
				fmt.Println("Error reading: ", err.Error())
				return
			}

			// Decode the JSON data
			json_data, err := JsonDecodeBytes(line)
			if err != nil {
				fmt.Println("Error converting JSON: ", err.Error())
				continue
			}

			// Get the topic from the JSON data
			topic, ok := json_data["topic"].(string)
			if !ok {
				fmt.Println("Topic missing")
				continue

			}
			if DEBUG_LEVEL >= 2 {
				fmt.Println("-->", topic, json_data)
			}

			// Take the lock so we wait until we have processed
			// the current packet
			// XXX we'll see if this causes any problems
			self.msg_block.Lock()

			var pub message.Publisher

			// NOTE: an unauthed user can only call user.auth
			if !self.authed && topic != "user.auth" {
				fmt.Println("User not authorised for", topic)
				continue

			} else if strings.HasPrefix(topic, "user.") {
				// Message for the user
				pub = self.pub_sub

			} else if strings.HasPrefix(topic, "server.") {
				// Message for the server
				pub = self.server.pub_sub

			} else if strings.HasPrefix(topic, "room.") {
				// Message for the current room
				if self.room == nil {
					continue
				}
				pub = self.room.pub_sub

			} else {
				// We don't know where to send this message
				fmt.Println("Unknown topic route ", topic)
				continue
			}

			msg := message.NewMessage(watermill.NewUUID(), line)
			msg.Metadata.Set("user-id", self.id)

			// Send packet into router
			middleware.SetCorrelationID(watermill.NewUUID(), msg)

			if err := pub.Publish(topic, msg); err != nil {
				fmt.Println("Error sending message: ", err.Error())
				continue
			}
			continue
		}

	}
}

func (self *User) Send_json(data Json) error {
	bytes, err := JsonEncodeBytes(data)
	if err != nil {
		return err
	}

	// Have to take lock until we finish writing
	self.mtx.Lock()
	defer self.mtx.Unlock()

	// Write packet mode
	self.writer.WriteByte(1)

	self.writer.Write(bytes)

	// Write newline at end
	self.writer.WriteByte('\n')

	self.writer.Flush()
	if DEBUG_LEVEL >= 2 {
		fmt.Println("<--", data)
	}

	return nil

}

func (self *User) Get_user(id string) *User {
	if id != self.id {
		return nil
	}
	return self
}

func (self *User) route(t string, h MessageHandler) {
	self.router.AddNoPublisherHandler(t, t, self.pub_sub, User_middleware(self, Message_wrapper(h)))
}

func (self *User) add_routes() {
	// Override this if you want to provide some other form of auth
	self.route("user.auth", self.simple_server_auth)
	self.route("user.ready.toggle", self.toggle_ready_handler)
	self.route("user.hard.toggle", self.toggle_hard_handler)
	self.route("user.mirror.toggle", self.toggle_mirror_handler)
	self.route("user.song.level", self.song_level_handler)
	self.route("user.nomap", self.no_map_handler)
}

func (self *User) simple_server_auth(msg *Message) error {

	version, ok := msg.Json()["version"].(string)
	var version_f float64
	if ok {
		var err error
		version_f, err = strconv.ParseFloat(version[1:], 64)
		ok = err == nil
	}
	if ok {
		ok = version_f >= 0.16
	}
	if !ok {
		self.Send_json(Json{
			"topic": "server.error",
			"error": "Server does not support this version of multiplayer",
		})
	}

	password := msg.Json()["password"].(string)

	if password != "d3e5a1c17644e28fa156" {
		self.Send_json(Json{
			"topic": "server.error",
			"error": "Not authorized",
		})
		return nil
	}

	name, ok := msg.Json()["name"].(string)

	if !ok {
		return nil
	}

	self.name = name

	self.authed = true
	if DEBUG_LEVEL >= 2 {
		fmt.Println("Authenticated user", self.name)
	}

	self.Send_json(Json{
		"topic":        "server.info",
		"userid":       self.id,
		"version":      VERSION,
		"refresh_rate": SCOREBOARD_REFERSH_RATE,
	})

	self.server.Send_rooms_to_user(self)

	return nil
}

func (self *User) song_level_handler(msg *Message) error {
	if self.room == nil || self.playing {
		return nil
	}

	new_level := Json_int(msg.Json()["level"])
	if new_level == self.level {
		return nil
	}

	self.level = new_level
	self.room.Send_lobby_update()
	return nil

}

func (self *User) toggle_ready_handler(msg *Message) error {
	if self.room == nil || self.playing {
		return nil
	}

	self.ready = !self.ready
	self.missing_map = false

	self.room.Send_lobby_update()
	return nil
}

func (self *User) no_map_handler(msg *Message) error {
	if self.room == nil || self.playing {
		return nil
	}
	if self.missing_map {
		return nil
	}

	self.missing_map = true

	self.room.Send_lobby_update()
	return nil
}

func (self *User) toggle_hard_handler(msg *Message) error {
	if self.room == nil || self.playing {
		return nil
	}
	self.hard_mode = !self.hard_mode
	self.room.Send_lobby_update_to_user(self)
	return nil
}

func (self *User) toggle_mirror_handler(msg *Message) error {
	if self.room == nil || self.playing {
		return nil
	}
	self.mirror_mode = !self.mirror_mode
	self.room.Send_lobby_update_to_user(self)
	return nil
}
