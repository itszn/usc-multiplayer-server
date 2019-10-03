package main

import (
	"crypto/rand"
	"fmt"
	"sort"
	"sync"
	"time"

	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/infrastructure/gochannel"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
	"github.com/ThreeDotsLabs/watermill/message/router/plugin"
	"github.com/google/uuid"
)

type Song struct {
	name  string
	hash  string
	diff  int
	level int
}

type Room struct {
	server *Server

	name string
	id   string

	max      int
	password string

	users    []*User
	user_map map[string]*User

	song        *Song
	host        *User
	owner       *User
	first_owner string

	do_rotate_host bool

	pub_sub message.PubSub
	router  *message.Router

	start_soon bool
	in_game    bool
	is_synced  bool

	mtx    sync.RWMutex
	mtx_id uint64

	join_token string
}

// TODO per room password
func New_room(server *Server, name string, max int, password string) *Room {
	room := &Room{
		server: server,
		name:   name,
		id:     uuid.New().String(),

		max: max,

		users:    make([]*User, 0),
		user_map: make(map[string]*User),
		password: password,

		song: nil,
		host: nil,

		do_rotate_host: false,
		start_soon:     false,
		in_game:        false,
		is_synced:      false,

		mtx_id: 1,
	}

	room.init()

	return room
}

// Lock the mutex and return the current key
func (self *Room) mtx_lock() uint64 {
	self.mtx.Lock()
	return self.mtx_id
}

// Try to unlock the mutex using a key
// if the key was already used, nop
func (self *Room) mtx_unlock(key uint64) {
	if self.mtx_id == key || key == 0 {
		// Update the key for the next lock
		self.mtx_id++
		self.mtx.Unlock()
	}
}

func (self *Room) Get_user(id string) *User {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	if user, ok := self.user_map[id]; ok {
		return user
	}
	return nil
}

func (self *Room) Num_users() int {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	return len(self.users)
}

func (self *Room) Rotate_host() {
	defer self.mtx_unlock(self.mtx_lock())

	user_len := len(self.users)
	if user_len < 2 {
		return
	}

	var ind int
	for i, u := range self.users {
		if u == self.host {
			ind = i
			break
		}
	}
	ind = (ind + 1) % user_len

	self.host = self.users[ind]

	self.mtx_unlock(0)
	self.Send_lobby_update()
}

func (self *Room) init() error {
	token_bytes := make([]byte, 16)
	rand.Read(token_bytes)
	self.join_token = fmt.Sprintf("%x", token_bytes)

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

func (self *Room) Start() {
	if err := self.router.Run(); err != nil {
		fmt.Println("Error ", err)
	}
}

func (self *Room) route(t string, h MessageHandler) {
	self.router.AddNoPublisherHandler(t, t, self.pub_sub, User_middleware(self, Message_wrapper(h)))
}

func (self *Room) add_routes() {
	self.route("room.leave", self.leave_room_handler)
	self.route("room.setsong", self.set_song_handler)

	self.route("room.option.rotation.toggle", self.toggle_rotate_handler)

	self.route("room.game.start", self.start_game_handler)
	self.route("room.sync.ready", self.handle_sync_ready)
	self.route("room.score.update", self.handle_game_score)
	self.route("room.score.final", self.handle_final_score)

	self.route("room.update.get", self.handle_back_in_lobby)
	self.route("room.host.set", self.handle_set_host)

	self.route("room.kick", self.handle_kick)
}

func (self *Room) Add_user(user *User) {
	defer self.mtx_unlock(self.mtx_lock())

	if _, ok := self.user_map[user.id]; ok {
		return
	}

	self.user_map[user.id] = user
	self.users = append(self.users, user)

	user.room = self
	user.score = nil
	user.ready = false
	user.playing = false
	user.missing_map = false
	user.hard_mode = false
	user.mirror_mode = false
	if self.song == nil {
		user.level = 0
	} else {
		user.level = self.song.level
	}

	user.score_list = make([]Score_point, 0)

	if self.owner == nil {
		self.owner = user
		self.first_owner = user.id
		self.host = user
	} else if self.first_owner == user.id {
		// Reclaim ownership if you leave and rejoin
		self.owner = user
	}
}

func (self *Room) Remove_user(user *User) {
	self.Remove_user_by_id(user.id)

}

func (self *Room) Remove_user_by_id(id string) {
	user, ok := self.user_map[id]
	if !ok {
		return
	}

	if self.host == user && len(self.users) > 1 {
		self.Rotate_host()
	}

	defer self.mtx_unlock(self.mtx_lock())

	user.room = nil

	// Remove the user from the lists
	delete(self.user_map, user.id)
	for i, u := range self.users {
		if u == user {
			self.users = append(self.users[:i], self.users[i+1:]...)
			break
		}
	}

	// Select anyone as the owner
	if self.owner == user && len(self.users) > 0 {
		// TODO Do we want to change owner or just have none?
		self.owner = self.users[0]
	}

	self.mtx_unlock(0)

	// XXX race with a user being added?
	if len(self.users) == 0 {
		self.destroy()
		return
	}

	self.Send_lobby_update()
	self.server.Send_rooms_to_users()
}

func (self *Room) destroy() {
	self.server.Remove_room(self)
	self.server.Send_rooms_to_users()

	self.router.Close()
}

func (self *Room) leave_room_handler(msg *Message) error {
	self.Remove_user(msg.User())
	return nil
}

func (self *Room) Send_lobby_update() {
	self.Send_lobby_update_to(self.users)
}
func (self *Room) Send_lobby_update_to_user(target_user *User) {
	target := make([]*User, 1)
	target[0] = target_user
	self.Send_lobby_update_to(target)
}

func (self *Room) Send_lobby_update_to(target_users []*User) {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	make_data := func(u *User) Json {
		data := Json{
			"name":        u.name,
			"id":          u.id,
			"ready":       u.ready || u == self.host,
			"missing_map": u.missing_map,
			"level":       u.level,
		}
		if u.score != nil {
			data["score"] = u.score.score
			data["combo"] = u.score.combo
			data["clear"] = u.score.clear
		}
		return data
	}

	// First add and sort anyone who has a score
	board := make([]score_sort, 0)
	for _, u := range self.users {
		if u.score == nil {
			continue
		}

		// clear_off makes sure people who cleared are higher in the leaderboard
		clear_off := uint32(0)
		if u.score.clear > 1 {
			clear_off = 10000000
		}
		board = append(board, score_sort{
			user:  u,
			score: u.score.score + clear_off,
		})
	}
	sort.Sort(score_sort_by_score(board))

	var userdata []Json
	for _, b := range board {
		userdata = append(userdata, make_data(b.user))
	}

	// Now get everyone else
	for _, u := range self.users {
		if u.score != nil {
			continue
		}
		userdata = append(userdata, make_data(u))
	}

	packet := Json{
		"topic":      "room.update",
		"users":      userdata,
		"do_rotate":  self.do_rotate_host,
		"start_soon": self.start_soon,
		"join_token": self.join_token,
	}
	if self.song != nil {
		packet["song"] = self.song.name
		packet["diff"] = self.song.diff
		packet["level"] = self.song.level
		packet["hash"] = self.song.hash
	} else {
		packet["song"] = nil
		packet["diff"] = nil
		packet["level"] = nil
		packet["hash"] = nil
	}

	if self.host != nil && !self.in_game {
		packet["host"] = self.host.id
	}
	packet["owner"] = self.owner.id

	for _, u := range target_users {
		if u.playing {
			continue
		}
		packet["hard_mode"] = u.hard_mode
		packet["mirror_mode"] = u.mirror_mode
		u.Send_json(packet)
	}
}

func (self *Room) set_song_handler(msg *Message) error {
	user := msg.User()

	if user != self.host {
		return nil
	}

	json := msg.Json()

	name := json["song"].(string)

	// If we picked the same song again, we don't have to
	// update missingmap and ready state of users
	if self.song != nil && self.song.name != name {
		for _, u := range self.users {
			u.ready = false
			u.missing_map = false
		}

	}

	song := &Song{
		name: name,
	}

	song.diff = Json_int(json["diff"])
	song.level = Json_int(json["level"])
	hash, has_hash := json["hash"].(string)
	if !has_hash {
		hash = ""
	}
	song.hash = hash
	user.level = song.level

	defer self.mtx_unlock(self.mtx_lock())

	self.song = song

	self.mtx_unlock(0)
	self.Send_lobby_update()

	return nil
}

func (self *Room) toggle_rotate_handler(msg *Message) error {
	user := msg.User()

	// Either host or owner can change this setting
	if user != self.host && user != self.owner {
		return nil
	}

	self.do_rotate_host = !self.do_rotate_host

	self.Send_lobby_update()
	return nil
}

func (self *Room) start_game_handler(msg *Message) error {
	user := msg.User()

	if user != self.host {
		return nil
	}

	self.start_soon = true

	self.Send_lobby_update()

	// TODO allow this to be canceled
	go func() {
		time.Sleep(5 * time.Second)

		defer self.mtx_unlock(self.mtx_lock())
		self.in_game = true
		self.is_synced = false
		self.start_soon = false

		for _, u := range self.users {
			u.score = nil
			// We shouldn't need to grab a mutex because the list won't be accessed until the game has already started
			u.score_list = make([]Score_point, 0)

			// If player is missing the map, they just stay in the lobby
			if u.missing_map {
				continue
			}

			u.Send_json(Json{
				"topic":  "game.started",
				"hard":   u.hard_mode,
				"mirror": u.mirror_mode,
			})
			u.playing = true
			u.ready = false
			u.synced = false
		}

		self.mtx_unlock(0)
		self.Send_lobby_update()

		// We want to prevent players from holding up
		time.Sleep(15 * time.Second)
		self.check_sync_state(true)
	}()

	return nil
}

func (self *Room) check_sync_state(force bool) error {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	if self.is_synced {
		return nil
	}

	if !force {
		// Check if all players are ready
		for _, u := range self.users {
			if !u.playing {
				continue
			}
			if !u.synced {
				return nil
			}
		}
	}

	self.is_synced = true

	go func() {
		time.Sleep(1 * time.Second)

		self.mtx.RLock()
		defer self.mtx.RUnlock()

		// Send sync start packet
		for _, u := range self.users {
			if !u.playing {
				continue
			}
			u.Send_json(Json{
				"topic": "game.sync.start",
			})

			// Start them but kick them off the scoreboard (so they don't lag behind)
			if !u.synced {
				u.playing = false
			}
		}
	}()
	return nil
}

func (self *Room) handle_sync_ready(msg *Message) error {
	user := msg.User()
	if !user.playing || user.synced {
		return nil
	}

	user.synced = true

	self.check_sync_state(false)

	return nil
}

type score_sort struct {
	user  *User
	score uint32
}

type score_sort_by_score []score_sort

func (self score_sort_by_score) Len() int {
	return len(self)
}

func (self score_sort_by_score) Swap(i, j int) {
	self[i].score, self[j].score = self[j].score, self[i].score
	self[i].user, self[j].user = self[j].user, self[i].user
}

func (self score_sort_by_score) Less(i, j int) bool {
	return self[i].score > self[j].score
}

func (self *Room) Update_scoreboard(user *User, new_time uint32) {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	for _, u := range self.users {
		if u == user || !u.playing {
			continue
		}
		last_time := u.Get_last_score_time()
		if last_time < new_time {
			// If there is a lowertime, this update doesn't change the scoreboard
			return
		}
	}
	board := make([]score_sort, 0)
	for _, u := range self.users {
		if !u.playing && u.score == nil {
			continue
		}
		board = append(board, score_sort{
			user:  u,
			score: u.Get_score_at(new_time),
		})
	}
	sort.Sort(score_sort_by_score(board))

	userdata := make([]Json, 0)
	for _, b := range board {
		userdata = append(userdata, Json{
			"name":  b.user.name,
			"id":    b.user.id,
			"score": b.score,
		})
	}
	packet := Json{
		"topic": "game.scoreboard",
		"users": userdata,
	}

	for _, b := range board {
		b.user.Send_json(packet)
	}
}

func (self *Room) handle_game_score(msg *Message) error {
	user := msg.User()
	if !user.playing {
		return nil
	}

	new_time := uint32(Json_int(msg.Json()["time"]))

	user.Add_new_score(
		uint32(Json_int(msg.Json()["score"])),
		new_time,
	)

	self.Update_scoreboard(user, new_time)

	return nil
}

func (self *Room) handle_final_score(msg *Message) error {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	user := msg.User()
	if !user.playing {
		return nil
	}

	new_time := ^uint32(0)
	score := uint32(Json_int(msg.Json()["score"]))
	user.Add_new_score(score, new_time)

	self.Update_scoreboard(user, new_time)

	user.score = &Score{
		score: score,
		combo: uint32(Json_int(msg.Json()["combo"])),
		clear: uint32(Json_int(msg.Json()["clear"])),
	}
	user.playing = false

	done := true
	for _, u := range self.users {
		if u.playing {
			done = false
			break
		}
	}

	if done {
		self.in_game = false
		self.is_synced = false
		if self.do_rotate_host {
			// Use go routine here to avoid deadlock
			go self.Rotate_host()
		}
	}
	user.ready = false

	self.Send_lobby_update()
	return nil
}

func (self *Room) handle_back_in_lobby(msg *Message) error {
	self.Send_lobby_update_to_user(msg.User())
	return nil
}

func (self *Room) handle_set_host(msg *Message) error {
	self.mtx.RLock()
	defer self.mtx.RUnlock()

	user := msg.User()
	// Either host or owner can change host
	if self.host != user && self.owner != user {
		return nil
	}

	uid := msg.Json()["host"].(string)
	new_host, ok := self.user_map[uid]
	if !ok {
		return nil
	}

	self.host = new_host

	self.Send_lobby_update()
	return nil
}

func (self *Room) handle_kick(msg *Message) error {
	user := msg.User()
	// Only owner can kick
	if self.owner != user {
		return nil
	}

	uid := msg.Json()["id"].(string)
	self.Remove_user_by_id(uid)
	return nil

}
