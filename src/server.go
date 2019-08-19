package main

import (
	"errors"
	"fmt"
	"net"
	"os"
	"os/signal"
	"sort"
	"sync"
	//	"time"

	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/infrastructure/gochannel"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
	"github.com/ThreeDotsLabs/watermill/message/router/plugin"
)

type Server struct {
	bind string

	pub_sub message.PubSub
	router  *message.Router

	users map[string]*User
	rooms map[string]*Room

	mtx sync.RWMutex
}

func New_server(bind string) *Server {
	server := &Server{
		bind:  bind,
		users: make(map[string]*User),
		rooms: make(map[string]*Room),
	}
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
	defer self.mtx.Unlock()

	self.users[u.id] = u
}

func (self *Server) Remove_user(u *User) {
	self.mtx.Lock()
	defer self.mtx.Unlock()

	delete(self.users, u.id)
}

func (self *Server) Add_room(r *Room) {
	self.mtx.Lock()
	defer self.mtx.Unlock()

	self.rooms[r.id] = r
}

func (self *Server) Remove_room(r *Room) {
	self.mtx.Lock()
	defer self.mtx.Unlock()

	delete(self.rooms, r.id)
}

func (self *Server) Start() {
	// Main pub_sub for server global events
	self.pub_sub = gochannel.NewGoChannel(gochannel.Config{}, logger)

	router, err := message.NewRouter(message.RouterConfig{}, logger)
	if err != nil {
		panic(err)
	}
	self.router = router

	// Add handler to shut down router
	router.AddPlugin(plugin.SignalsHandler)

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
	defer self.mtx.RUnlock()

	rooms := make([]*Room, 0, len(self.rooms))
	for _, room := range self.rooms {
		rooms = append(rooms, room)
	}
	sort.Sort(room_sort(rooms))

	var roomdata []Json
	for _, room := range rooms {
		roomdata = append(roomdata, Json{
			"current": room.Num_users(),
			"max":     room.max,
			"name":    room.name,
			"ingame":  room.in_game,
			"id":      room.id,
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

	room.Add_user(user)

	user.Send_json(Json{
		"topic": "server.room.joined",
		"room": Json{
			"current": room.Num_users(),
			"max":     room.max,
			"name":    room.name,
			"ingame":  room.in_game,
			"id":      room.id,
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

	// TODO add room password or something
	room_id := msg.Json()["id"].(string)

	self.mtx.RLock()
	defer self.mtx.RUnlock()

	room, ok := self.rooms[room_id]
	if !ok {
		return errors.New("Room not found")
	}

	return self.add_user_to_room(user, room)
}

func (self *Server) new_room_handler(msg *Message) error {
	user := msg.User()
	if user.room != nil {
		return nil
	}

	name := user.name + "'s Game"
	room := New_room(self, name, 10)
	self.Add_room(room)
	go room.Start()

	return self.add_user_to_room(user, room)
}
