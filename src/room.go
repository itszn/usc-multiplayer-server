package main

import (
	crand "crypto/rand"
	"fmt"
	"sort"
	"time"
	"strings"
	"strconv"
	"math/rand"

	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/infrastructure/gochannel"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
	"github.com/google/uuid"
)

type Song struct {
	name  string
	audio_hash  string
	chart_hash string
	diff  int
	level int
}

type Room struct {
	server *Server

	alive bool

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

	last_time uint32

	mtx Lock

	join_token string

	is_event_room bool
}

// TODO per room password
func New_room(server *Server, name string, max int, password string, is_event_room bool) *Room {
	room := &Room{
		server: server,
		name:   name,
		id:     uuid.New().String(),

		max: max,

		alive: true,

		users:    make([]*User, 0),
		user_map: make(map[string]*User),
		password: password,

		song: nil,
		host: nil,

		do_rotate_host: false,
		start_soon:     false,
		in_game:        false,
		is_synced:      false,

		is_event_room: is_event_room,
	}

	room.mtx.init(1)

	room.init()

	return room
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
	self.mtx.RLock()
	user_len := len(self.users)

	if user_len < 2 {
		self.mtx.Unlock()
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
	self.mtx.RUnlock()

	self.Send_lobby_update()
}

func (self *Room) init() error {
	token_bytes := make([]byte, 16)
	crand.Read(token_bytes)
	self.join_token = fmt.Sprintf("%x", token_bytes)

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

	self.route("room.chat.send", self.chat_handler)
}

func (self *Room) Add_user(user *User) bool {
	self.mtx.Lock()
	defer func() {
		self.mtx.Unlock()
		if len(self.users) == 1 {
			bot, _ := New_user(nil, self.server)
			self.server.Add_user(bot)
			self.Add_user(bot);
		}
	}()

	if !self.alive {
		return false
	}

	// TODO this counts replay users but probably shouldn't
	// Its not a problem atm bc replay only supports two players anyway
	// So you can have at a max 4 refs
	if len(self.users) == self.max {
		return false
	}

	if _, ok := self.user_map[user.id]; ok {
		return false
	}

	// Normal users can only join normal rooms
	if (self.is_event_room && !user.is_event_user) {
		return false
	}
	if (!self.is_event_room && user.is_playback) {
		return false
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

	// Only try and match users if in event room
	if (self.is_event_room) {
		self.Match_replay_user(user)
	}

	return true
}

func (self *Room) Match_replay_user(user *User) {
	// Refs don't get matched
	if (user == nil || user.is_ref) {
		return
	}

	user.replay_user = nil
	// Try to match a playback user with a normal user and vis-versa
	for _, u := range self.users {
		// Can't match with themselves
		if u == user {
			continue
		}
		// If this user is already matched or is a ref, skip
		if u.is_ref || u.replay_user != nil {
			continue
		}

		// If user is also normal or also playback skip
		if u.is_playback == user.is_playback {
			continue
		}

		// Match the users and let users know
		u.replay_user = user
		user.replay_user = u
		fmt.Printf("Playback: %s <-> %s\n", u.name, user.name)
		if user.is_playback {
			self.Broadcast_chat(
				fmt.Sprintf("> Streaming enabled for %s <",u.name))
		} else {
			self.Broadcast_chat(
				fmt.Sprintf("> Streaming enabled for %s <",user.name))
		}
		break
	}
}

func (self *Room) Remove_user(user *User) {
	self.Remove_user_by_id(user.id)

}

func (self *Room) Remove_user_by_id(id string) {
	self.mtx.Lock()

	user, ok := self.user_map[id]

	if !ok {
		self.mtx.Unlock()
		return
	}

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

	if len(self.users) == 1 {
		us := self.users[0]
		if (us.bot) {
			us.room = nil
			delete(self.user_map, us.id)
			for i, u := range self.users {
				if u == us {
					self.users = append(self.users[:i], self.users[i+1:]...)
					break
				}
			}
		}
	}

	if len(self.users) == 0 {
		self.destroy()
	}

	// Unlink replay user
	if user.replay_user != nil {
		if user.is_playback {
			self.Broadcast_chat(
				fmt.Sprintf("> Streaming lost for %s <",user.replay_user.name))
		} else {
			self.Broadcast_chat(
				fmt.Sprintf("> Streaming lost for %s <",user.name))
		}
		self.Match_replay_user(user.replay_user)
		user.replay_user = nil
	}

	self.mtx.Unlock()

	if !self.alive {
		// We need to ensure we are not in a lock atm
		self.server.Remove_room(self)
	} else {

		self.mtx.RLock()
		if self.host == user && len(self.users) > 1 {
			self.Rotate_host()
		}

		self.Send_lobby_update()
		self.mtx.RUnlock()
	}

	// We need to ensure we are not in a lock atm
	self.server.Send_rooms_to_users()
}

func (self *Room) destroy() {
	self.alive = false
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

	make_data := func(u *User) Json {
		name := u.name
		if u.is_ref {
			name = "[ref] "+name
		}
		data := Json{
			"name":        name,
			"id":          u.id,
			"ready":       u.ready || u == self.host,
			"missing_map": u.missing_map,
			"level":       u.level,
		}
		if u.score != nil && !u.is_ref{
			data["score"] = u.score.score
			data["combo"] = u.score.combo
			data["clear"] = u.score.clear
		}
		if u.extra_data != "" {
			data["extra_data"] = u.extra_data
		}
		return data
	}

	// First add and sort anyone who has a score
	board := make([]score_sort, 0)

	self.mtx.RLock()

	for _, u := range self.users {
		if u.is_playback || u.score == nil || u.is_ref {
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
		if u.is_playback || (u.score != nil && !u.is_ref) {
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
		packet["hash"] = self.song.audio_hash
		packet["audio_hash"] = self.song.audio_hash
		packet["chart_hash"] = self.song.chart_hash
	} else {
		packet["song"] = nil
		packet["diff"] = nil
		packet["level"] = nil
		packet["hash"] = nil
		packet["audio_hash"] = nil
		packet["chart_hash"] = nil
	}

	if self.host != nil && !self.in_game {
		packet["host"] = self.host.id
	}
	packet["owner"] = self.owner.id

	self.mtx.RUnlock()

	for _, u := range target_users {
		if u.playing {
			continue
		}
		packet["hard_mode"] = u.hard_mode
		packet["mirror_mode"] = u.mirror_mode
		if u.replay_user != nil {
			packet["replay_name"] = u.replay_user.name
			packet["replay_id"] = u.replay_user.id
		} else {
			packet["replay_name"] = ""
			packet["replay_id"] = ""
		}
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

	chart_hash, has_hash := json["chart_hash"].(string)
	if !has_hash {
		chart_hash = ""
	}
	song.chart_hash = chart_hash

	audio_hash, has_hash := json["chart_hash"].(string)
	if !has_hash {
		audio_hash, has_hash = json["hash"].(string)
		if !has_hash {
			audio_hash = ""
		}
	}
	song.audio_hash = audio_hash

	user.level = song.level

	self.mtx.Lock()
	self.song = song
	self.mtx.Unlock()

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

		self.mtx.Lock()
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

			packet := Json{
				"topic":  "game.started",
			}
			if u.is_playback && u.replay_user != nil {
				packet["replay_level"] = u.replay_user.level
				packet["hard"] = u.replay_user.hard_mode
				packet["mirror"] = u.replay_user.mirror_mode
			} else {
				packet["hard"] = u.hard_mode
				packet["mirror"] = u.mirror_mode
			}

			u.Send_json(packet)
			u.playing = true
			u.ready = false
			u.synced = false
		}
		// Ok because we only called user locking functions
		self.mtx.Unlock()

		self.Send_lobby_update()

		// We want to prevent players from holding up
		time.Sleep(15 * time.Second)
		self.check_sync_state(true)
	}()

	return nil
}

func (self *Room) check_sync_state(force bool) error {
	self.mtx.RLock()
	// Ok because we only call user locking  functions
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

	// Start the game for real
	go func() {
		time.Sleep(1 * time.Second)

		self.mtx.RLock()
		// Ok because we only call user locking functions
		defer self.mtx.RUnlock()

		var userdata []Json
		for _, u := range self.users {
			if u.is_playback || !u.playing || u.is_ref {
				continue
			}
			data := Json{
				"name":        u.name,
				"id":          u.id,
				"ready":       true,
				"missing_map": false,
				"level":       u.level,
			}
			if u.extra_data != "" {
				data["extra_data"] = u.extra_data
			}
			userdata = append(userdata, data)
		}

		packet := Json{
			"topic": "game.sync.start",
			"users": userdata,
		}

		has_playback := false

		// Send sync start packet
		for _, u := range self.users {
			if !u.playing {
				continue
			}
			if u.is_playback {
				has_playback = true
				continue
			}
			u.Send_json(packet)

			// Start them but kick them off the scoreboard (so they don't lag behind)
			if !u.synced {
				u.playing = false
			}
		}

		if !has_playback {
			return
		}

		// Kick off the playback users
		time.Sleep(10 * time.Second)
		for _, u := range self.users {
			if !u.playing || !u.is_playback {
				continue
			}
			u.Send_json(packet)
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
	// Ok because we only call user locking functions
	defer self.mtx.RUnlock()

	for _, u := range self.users {
		if u.is_playback || u == user || !u.playing || u.is_ref {
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
		if u.is_playback || (!u.playing && u.score == nil) || u.is_ref {
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

	// Also send to refs who are playing
	for _, u := range self.users {
		if !u.is_ref || !u.playing {
			continue
		}
		u.Send_json(packet)
	}
}

func (self *Room) handle_game_score(msg *Message) error {
	user := msg.User()
	if !user.playing || user.is_playback || user.is_ref {
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

	user := msg.User()
	if !user.playing {
		return nil
	}

	user.playing = false

	var done bool
	if user.is_ref || user.is_playback {

		self.mtx.RLock()
		defer self.mtx.RUnlock()

		done = true
		for _, u := range self.users {
			if u.playing && !u.is_playback && !u.bot {
				done = false
			}
			if u.bot {
				u.ready = false
				u.playing = false
				u.score = &Score{
					score: u.last_score,
					combo: uint32(0),
					clear: uint32(2),
				}
			}
		}
	} else {
		new_time := ^uint32(0)
		score := uint32(Json_int(msg.Json()["score"]))

		user.Add_new_score(score, new_time)

		self.Update_scoreboard(user, new_time)

		user.score = &Score{
			score: score,
			combo: uint32(Json_int(msg.Json()["combo"])),
			clear: uint32(Json_int(msg.Json()["clear"])),
		}

		final_stats := msg.Json()
		final_stats["uid"] = user.id
		final_stats["name"] = user.name
		final_stats["topic"] = "game.finalstats"


		self.mtx.RLock()
		defer self.mtx.RUnlock()

		_, has_stats := final_stats["gauge"]

		if has_stats {
			// validate type of stats
			Json_float(final_stats["gauge"])
			Json_float(final_stats["mean_delta"])
			Json_float(final_stats["median_delta"])
			Json_int(final_stats["early"])
			Json_int(final_stats["late"])
			Json_int(final_stats["miss"])
			Json_int(final_stats["near"])
			Json_int(final_stats["crit"])
			Json_int(final_stats["flags"])
			samples := final_stats["graph"].([]interface{})
			if len(samples) < 256 {
				panic("Not enough graph samples")
			} else {
				for _, s := range samples {
					Json_float(s)
				}
			}
		}

		done = true
		for _, u := range self.users {
			if u.playing && !u.is_playback && !u.bot{
				done = false
			}
			if has_stats && u.id != user.id && (u.playing || u.score != nil) {
				u.Send_json(final_stats)
			}
			if u.bot {
				u.playing = false
				u.ready = false
				u.score = &Score{
					score: u.last_score,
					combo: uint32(0),
					clear: uint32(2),
				}
			}
		}
	}

	if done {
		self.in_game = false
		self.is_synced = false
		if self.do_rotate_host {
			self.Rotate_host()
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

func (self *Room) Send_chat(user *User, msg string) {
	packet := Json{
		"topic": "server.chat.received",
		"message": msg,
	}
	user.Send_json(packet);
}

// Send a message to everyone
func (self *Room) Broadcast_chat(msg string) {
	packet := Json{
		"topic": "server.chat.received",
		"message": msg,
	}

	for _, u := range self.users {
		u.Send_json(packet)
	}
}

// Send a message to everyone but one user
func (self *Room) Mirror_chat(user *User, msg string) {
	packet := Json{
		"topic": "server.chat.received",
		"message": msg,
	}

	for _, u := range self.users {
		if u.id == user.id {
			continue
		}
		u.Send_json(packet)
	}
}

func (self *Room) chat_handler(msg *Message) error {
	user := msg.User()

	message := msg.Json()["message"].(string)

	if strings.HasPrefix(message, "/help") {
		self.Send_chat(user, "* /help  - This help text")
		self.Send_chat(user, "* /ref  - Mark yourself as a ref")
		self.Send_chat(user, "* /player  - Mark yourself as a player")
		self.Send_chat(user, "* /roll <num>  - Roll a number between 1 and <num>")
		return nil

	} else if message == "/ref" {
		if user.is_ref {
			self.Send_chat(user, "* You are already marked as a ref")
			return nil
		}
		// User is a ref and not a normal user
		user.is_ref = true;

		self.Broadcast_chat(
			fmt.Sprintf("> %s has marked themselves as a ref <",user.name))

		// Normally just send update and return
		if (!self.is_event_room) {
			self.Send_lobby_update();
			return nil
		}

		// For event room, unlink replay user
		if user.replay_user != nil {
			if user.is_playback {
				self.Broadcast_chat(
					fmt.Sprintf("> Streaming lost for %s <",user.replay_user.name))
			} else {
				self.Broadcast_chat(
					fmt.Sprintf("> Streaming lost for %s <",user.name))
			}
			self.Match_replay_user(user.replay_user)
			user.replay_user = nil
		}

		self.Send_lobby_update();
		return nil

	} else if message == "/player" {
		if !user.is_ref {
			self.Send_chat(user, "* You are already marked as a player")
			return nil
		}

		// User is a ref and not a normal user
		user.is_ref = false;

		self.Broadcast_chat(
			fmt.Sprintf("> %s has marked themselves as a player <",user.name))

		// For event room, relink user to replay
		if (self.is_event_room) {
			self.Match_replay_user(user)
		}

		self.Send_lobby_update();
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
