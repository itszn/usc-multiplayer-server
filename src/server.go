package main

import (
	"errors"
	"fmt"
	"net"
	"os"
	"os/signal"
	"sort"
	"strings"
	"strconv"
	"math/rand"
	//	"time"

	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/infrastructure/gochannel"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
)

type Server struct {
	bind string

	pub_sub message.PubSub
	router  *message.Router

	users map[string]*User
	rooms map[string]*Room

	mtx Lock
}

func New_server(bind string) *Server {
	server := &Server{
		bind:  bind,
		users: make(map[string]*User),
		rooms: make(map[string]*Room),
	}
	server.mtx.init(0)
	return server
}

func (self *Server) Get_user(id string) *User {
	if user, ok := self.users[id]; ok {
		return user
	}
	return nil
}

func (self *Server) listen_tcp() {
	l, err := net.Listen("tcp", self.bind)
	if err != nil {
		fmt.Println("Error listening:", err.Error())
		os.Exit(1)
	}

	defer l.Close()

	go func() {
		// Use to catch ^C
		sigchan := make(chan os.Signal, 1)
		signal.Notify(sigchan, os.Interrupt)

		<-sigchan

		l.Close()
	}()

	for {
		conn, err := l.Accept()
		if err != nil {
			fmt.Println("Error Accepting:", err.Error())
			return
		}

		if DEBUG_LEVEL >= 2 {
			fmt.Println("New connection")
		}

		new_user, err := New_user(conn, self)
		if err != nil {
			conn.Close()
			continue
		}

		self.Add_user(new_user)

		// Start pub/sub user router
		go new_user.Start()
	}
}

func (self *Server) Add_user(u *User) {
	self.mtx.Lock()

	self.users[u.id] = u

	self.mtx.Unlock()
}

func (self *Server) Remove_user(u *User) {
	self.mtx.Lock()

	delete(self.users, u.id)

	self.mtx.Unlock()
}

func (self *Server) Add_room(r *Room) {
	self.mtx.Lock()

	self.rooms[r.id] = r

	self.mtx.Unlock()
}

func (self *Server) Remove_room(r *Room) {
	self.mtx.Lock()

	delete(self.rooms, r.id)

	self.mtx.Unlock()
}

func (self *Server) Start() {
	// Main pub_sub for server global events
	self.pub_sub = gochannel.NewGoChannel(gochannel.Config{}, logger)

	router, err := message.NewRouter(message.RouterConfig{}, logger)
	if err != nil {
		panic(err)
	}
	self.router = router

	router.AddMiddleware(
		// correlation ID will copy correlation id from consumed message metadata to produced messages
		middleware.CorrelationID,

		Error_middleware,

		Panic_middleware,

		Json_middleware,
	)

	self.add_routes()

	go router.Run()
	self.listen_tcp()
}

func (self *Server) user_route(t string, h MessageHandler) {
	self.router.AddNoPublisherHandler(t, t, self.pub_sub,
		User_middleware(self, Message_wrapper(h)))
}

func (self *Server) add_routes() {
	// Routes users can talk to
	self.user_route("server.rooms", self.send_rooms_handler)
	self.user_route("server.room.join", self.join_room_handler)
	self.user_route("server.room.new", self.new_room_handler)
	self.user_route("server.chat.send", self.chat_handler)
}

type room_sort []*Room

func (self room_sort) Len() int {
	return len(self)
}
func (self room_sort) Swap(i, j int) {
	self[i], self[j] = self[j], self[i]
}
func (self room_sort) Less(i, j int) bool {
	return self[i].name < self[j].name
}

func (self *Server) Send_rooms_to_user(u *User) error {
	target := make(map[string]*User)
	target[u.id] = u
	return self.Send_rooms_to_targets(target)
}

func (self *Server) Send_rooms_to_users() error {
	return self.Send_rooms_to_targets(self.users)
}

// We take a map just because we don't want to copy all users every time
func (self *Server) Send_rooms_to_targets(users map[string]*User) error {
	self.mtx.RLock()

	rooms := make([]*Room, 0, len(self.rooms))
	for _, room := range self.rooms {
		rooms = append(rooms, room)
	}
	sort.Sort(room_sort(rooms))

	self.mtx.RUnlock()

	var roomdata []Json
	for _, room := range rooms {
		if !room.alive {
			continue
		}

		name := room.name
		if room.is_event_room {
			name = name + " <event>"
		}
		roomdata = append(roomdata, Json{
			"current":  room.Num_users(),
			"max":      room.max,
			"name":     name,
			"ingame":   room.in_game,
			"id":       room.id,
			"password": room.password != "",
		})
	}

	for _, user := range users {
		if user.room != nil {
			continue
		}
		user.Send_json(Json{
			"topic": "server.rooms",
			"rooms": roomdata,
		})
	}

	return nil
}

func (self *Server) send_rooms_handler(msg *Message) error {
	return self.Send_rooms_to_user(msg.User())
}

func (self *Server) add_user_to_room(user *User, room *Room) error {
	if room.Num_users() >= room.max {
		return errors.New("Room full")
	}

	if !room.Add_user(user) {
		return errors.New("Could not join room")
	}


	user.Send_json(Json{
		"topic": "server.room.joined",
		"room": Json{
			"current":    room.Num_users(),
			"max":        room.max,
			"name":       room.name,
			"ingame":     room.in_game,
			"id":         room.id,
			"password":   room.password != "",
			"join_token": room.join_token,
		},
	})

	self.Send_rooms_to_users()

	room.Send_lobby_update()
	return nil
}

func (self *Server) join_room_handler(msg *Message) error {
	user := msg.User()

	if user.room != nil {
		return errors.New("User already in a room")
	}

	join_token, has_token := msg.Json()["token"].(string)


	var room *Room = nil
	var found_room bool

	self.mtx.RLock()

	if has_token {
		for _, r := range self.rooms {
			if r.join_token == join_token {
				room = r
				break
			}
		}
		if room == nil {
			found_room = false
		} else {
			found_room = true
		}
	} else {
		room_id := msg.Json()["id"].(string)
		room, found_room = self.rooms[room_id]
	}

	self.mtx.RUnlock()

	if !found_room {
		return errors.New("Room not found")
	}

	if !has_token && room.password != "" {
		if room.password != msg.Json()["password"].(string) {
			user.Send_json(Json{
				"topic": "server.room.badpassword",
			})
			return nil
		}
	}

	return self.add_user_to_room(user, room)
}

func (self *Server) new_room_handler(msg *Message) error {
	user := msg.User()
	if user.room != nil {
		return nil
	}

	name, has_name := msg.Json()["name"].(string)
	if !has_name {
		name = user.name + "'s Game"
	}
	password, has_pass := msg.Json()["password"].(string)
	if !has_pass {
		password = ""
	}
	room := New_room(self, name, 8, password, user.is_event_user)
	self.Add_room(room)
	go room.Start()

	return self.add_user_to_room(user, room)
}

func (self *Server) Send_chat(user *User, msg string) {
	packet := Json{
		"topic": "server.chat.received",
		"message": msg,
	}
	user.Send_json(packet);
}

// Send a message to everyone
func (self *Server) Broadcast_chat(msg string) {
	packet := Json{
		"topic": "server.chat.received",
		"message": msg,
	}

	for _, u := range self.users {
		if u.room != nil {
			continue
		}
		u.Send_json(packet)
	}
}

// Send a message to everyone but one user
func (self *Server) Mirror_chat(user *User, msg string) {
	packet := Json{
		"topic": "server.chat.received",
		"message": msg,
	}

	for _, u := range self.users {
		if u.room != nil || u.id == user.id {
			continue
		}
		u.Send_json(packet)
	}
}

func (self *Server) chat_handler(msg *Message) error {
	user := msg.User()

	message := msg.Json()["message"].(string)

	if strings.HasPrefix(message, "/help") {
		self.Send_chat(user, "* /help  - This help text")
		self.Send_chat(user, "* /roll <num>  - Roll a number between 1 and <num>")
		return nil

	} else if strings.HasPrefix(message, "/roll") {
		// Roll a die
		s := strings.Split(message, " ")
		end := int(100)
		if len(s) > 1 {
			i, err := strconv.Atoi(s[1])
			if err == nil && i >= 1 {
				end = i
			}
		}
		res := rand.Intn(end) + 1

		self.Mirror_chat(user, fmt.Sprintf("[%s] %s",user.name, message))

		self.Broadcast_chat(
			fmt.Sprintf("> %s rolled a %d <",user.name, res))

		return nil
	}

	self.Mirror_chat(user, fmt.Sprintf("[%s] %s",user.name, message))
	return nil
}
