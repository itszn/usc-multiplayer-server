package main

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"sync"
	"time"
	"log"

	"github.com/ThreeDotsLabs/watermill"
	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/infrastructure/gochannel"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
	"github.com/google/uuid"

	"database/sql"
	_ "github.com/mattn/go-sqlite3"
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
	db *sql.DB

	score_list []Score_point

	ready       bool
	missing_map bool
	synced      bool

	playing     bool
	hard_mode   bool
	mirror_mode bool
	level       int
	last_hash   string

	last_score uint32
	last_time uint32

	is_ref bool

	conn   net.Conn
	reader *bufio.Reader
	writer *bufio.Writer

	pub_sub message.PubSub
	router  *message.Router

	mtx Lock

	// Used to block new messages from being processed
	msg_block sync.Mutex

	extra_data string

	is_playback bool
	replay_user *User
	is_event_user bool

	bot bool
}

func New_user(conn net.Conn, server *Server) (*User, error) {
	db,err := sql.Open("sqlite3", "./maps.db")
	if err != nil {
		log.Fatal(err)
	}
	user := &User{
		id:     uuid.New().String(),
		authed: false,

		db: db,

		server: server,
		room:   nil,

		name: "",

		score:       nil,
		ready:       false,
		missing_map: false,
		playing:     false,

		is_ref: false,

		conn:   conn,
		reader: bufio.NewReader(conn),
		writer: bufio.NewWriter(conn),

		extra_data: "",

		is_playback: false,
		replay_user: nil,
		is_event_user: false,
		bot: false,
	}
	user.mtx.init(2)

	if (conn == nil) {
		user.bot = true
		user.name = "itszn"
		user.authed = true
		return user, nil
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

	self.score_list = append(self.score_list, Score_point{
		score: score,
		time:  time,
	})

	self.last_score = score

	self.mtx.Unlock()
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
	fmt.Println("Remving user")
	self.server.Remove_user(self)
	fmt.Println("removed user")

	if self.room != nil {
		self.room.Remove_user(self)
	}

	if self.bot {
		return
	}

	self.db.Close()

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
			if self.name != "" {
				fmt.Println("User disconnected:", self.name)
			}
			return
		}

		// Newline delinitated JSON
		if mode == 1 {
			line, err := self.reader.ReadBytes('\n')
			if err != nil {
				fmt.Println("Error reading packet:", err.Error())
				return
			}

			// Decode the JSON data
			json_data, err := JsonDecodeBytes(line)
			if err != nil {
				fmt.Println("Error converting JSON:", err.Error())
				continue
			}

			// Get the topic from the JSON data
			topic, ok := json_data["topic"].(string)
			if !ok {
				fmt.Println("Error: Topic missing")
				continue

			}
			if (DEBUG_LEVEL >= 2 && topic != "room.score.update") || DEBUG_LEVEL >= 3 {
				fmt.Println("-->", topic, json_data)
			}

			// Take the lock so we wait until we have processed
			// the current packet
			// XXX we'll see if this causes any problems
			self.msg_block.Lock()

			var pub message.Publisher

			// NOTE: an unauthed user can only call user.auth
			if !self.authed && topic != "user.auth" {
				fmt.Println("Error: User not authorised for", topic)
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
				fmt.Println("Error: Unknown topic route", topic)
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
		} else if mode == 2 {
			// Length prefix
			var length uint32
			binary.Read(self.reader, binary.LittleEndian, &length)

			data := make([]byte, length, length)
			io.ReadFull(self.reader, data)
			if DEBUG_LEVEL >= 3 {
				fmt.Println("-->", data)
			}

			if self.replay_user != nil {
				self.replay_user.Send_length_prefix(data)
			}
		}

	}
}

func (self *User) Send_length_prefix(bytes []byte) error {
	// Have to take lock until we finish writing
	self.mtx.Lock()
	defer self.mtx.Unlock()

	// XXX we should not be able to hit this in this chal
	if self.bot {
		return nil
	}

	// Write packet mode
	self.writer.WriteByte(2)
	err := binary.Write(self.writer, binary.LittleEndian, uint32(len(bytes)))
	if err != nil {
		return err
	}
	self.writer.Write(bytes)
	self.writer.Flush()

	if DEBUG_LEVEL >= 3 {
		fmt.Println("<--", bytes)
	}

	return nil
}

func (self *User) Send_json(data Json) error {
	bytes, err := JsonEncodeBytes(data)
	if err != nil {
		return err
	}

	topic, ok := data["topic"].(string)
	if self.bot {
		if (DEBUG_LEVEL >= 2) {
			fmt.Println("BOT <--", data)
		}

		if topic == "room.update" {
			song, ok := data["song"].(string)
			if (ok) {
				sqlStmt := `SELECT DISTINCT folderId FROM Charts WHERE path LIKE "%` +  song + `%"`
				log.Print(sqlStmt)
				rows, serr := self.db.Query(sqlStmt)
				if serr != nil {
					log.Fatal(serr)
					self.no_map_handler(nil)
					return nil
				}
				defer rows.Close()
				found := false
				for rows.Next() {
					var id int
					log.Print(id)
					rows.Scan(&id)
					found = true
				}

				hash, _ := data["chart_hash"].(string)

				if hash == self.last_hash {
					return nil
				}

				if found {
					self.last_hash = hash
					if !self.ready {
						go func() {
							time.Sleep(2 * time.Second)
							log.Print(`found...`)
							self.toggle_ready_handler(nil)
						}()
					}
				} else {
					log.Print("BOT no map")
					self.ready = false
					self.no_map_handler(nil)
				}
			}
		}
		if topic == "game.started" {
			go func() {
				time.Sleep(1 * time.Second)
				fm := Make_fake_msg(self, Json{})
				self.room.handle_sync_ready(fm)
			}()
		}
		if topic == "game.sync.start" {
			self.room.handle_game_score(Make_fake_msg(self, Json{
				"score": int(0),
				"time": int(500),
			}))
			self.last_time = 0
		}
		if topic == "game.scoreboard" {
			t := self.room.host.Get_last_score_time()
			fmt.Println("t = ", t, self.last_time)
			if t == self.last_time {
				return nil
			}
			self.last_time = t

			s :=self.room.host.last_score + 132641
			if s > 10000000 {
				s = 10000000
			}

			self.room.handle_game_score(Make_fake_msg(self, Json{
				"score": int(s),
				"time": int(t+500),
			}))
		}


		return nil
	}

	// Have to take lock until we finish writing
	self.mtx.Lock()

	// Write packet mode
	self.writer.WriteByte(1)

	self.writer.Write(bytes)

	// Write newline at end
	self.writer.WriteByte('\n')

	self.writer.Flush()

	self.mtx.Unlock()

	if !ok {
		fmt.Println("<--", data)
		fmt.Println("Error: Topic missing")
	} else if (DEBUG_LEVEL >= 2 && topic != "game.scoreboard") || DEBUG_LEVEL >= 3{
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
	self.route("user.extra.set", self.set_extra_data_handler)
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

	if SERVER_PASS != "" {

		password := msg.Json()["password"].(string)

		if password != SERVER_PASS {
			self.Send_json(Json{
				"topic": "server.error",
				"error": "Not authorized",
			})
			return nil
		}
	}

	name, ok := msg.Json()["name"].(string)

	if !ok {
		return nil
	}

	self.name = name

	self.authed = true
	fmt.Println("User joined: ", self.name)

	is_playback, ok := msg.Json()["playback"].(bool)
	if !ok {
		is_playback = false
	}
	self.is_playback = is_playback

	is_event, ok := msg.Json()["eventclient"].(bool)
	if ok && is_event {
		self.Send_json(Json{
			"topic": "server.error",
			"error": "Server does not support event clients",
		})
	}

	self.Send_json(Json{
		"topic":        "server.info",
		"userid":       self.id,
		"version":      PROTO_VERSION,
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
	log.Print("no map")
	if self.room == nil || self.playing {
		log.Print("no room")
		return nil
	}
	if self.missing_map {
		log.Print("nomap already set")
		return nil
	}

	self.missing_map = true

	log.Print("sending lobby update")
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

func (self *User) set_extra_data_handler(msg *Message) error {
	self.extra_data = msg.Json()["data"].(string)

	if self.room != nil {
		// Update current room with your extra data
		self.room.Send_lobby_update()
	}
	return nil
}
