package main

import (
	"context"
	"errors"
	"fmt"

	"github.com/ThreeDotsLabs/watermill/message"
)

type key string

var (
	json_key = key("json_key")
	user_key = key("user_key")
)

type MsgRes []*message.Message

func Json_middleware(f message.HandlerFunc) message.HandlerFunc {
	return func(msg *message.Message) ([]*message.Message, error) {

		data, err := JsonDecodeBytes(msg.Payload)
		if err != nil {
			return nil, err
		}

		ctx := context.WithValue(msg.Context(), json_key, data)
		msg.SetContext(ctx)

		return f(msg)
	}
}

type UserProvider interface {
	Get_user(string) *User
}

func User_middleware(provider UserProvider, f message.HandlerFunc) message.HandlerFunc {
	return func(msg *message.Message) ([]*message.Message, error) {
		user := provider.Get_user(msg.Metadata.Get("user-id"))

		if user == nil {
			return nil, errors.New("No user provided to user middleware")
		}

		ctx := context.WithValue(msg.Context(), user_key, user)
		msg.SetContext(ctx)

		// Tell the user go-routine we are done with this message
		defer user.Unblock()

		return f(msg)
	}
}

type Message struct {
	*message.Message
}

func Make_fake_msg(user *User, data Json) *Message {
	msg := &Message{message.NewMessage("asdf",[]byte{})}
	ctx := context.WithValue(msg.Context(), user_key, user)
	msg.SetContext(ctx)

	ctx = context.WithValue(msg.Context(), json_key, data)
	msg.SetContext(ctx)

	fmt.Println("BOT -->", data)

	return msg
}

type MessageHandler func(*Message) error

func Message_wrapper(f MessageHandler) message.HandlerFunc {
	return func(m *message.Message) ([]*message.Message, error) {
		err := f(&Message{m})
		return nil, err
	}
}

func (self *Message) Json() Json {
	return self.Context().Value(json_key).(Json)
}

func (self *Message) User() *User {
	return self.Context().Value(user_key).(*User)
}

func Error_middleware(f message.HandlerFunc) message.HandlerFunc {
	return func(msg *message.Message) ([]*message.Message, error) {
		res, err := f(msg)
		if err != nil {
			fmt.Println("Message rejected:", err.Error())
		}
		return res, nil
	}
}

func Panic_middleware(f message.HandlerFunc) message.HandlerFunc {
	return func(msg *message.Message) (res []*message.Message, err error) {
		defer func() {
			if r := recover(); r != nil {
				err = fmt.Errorf("%v", r)
			}
		}()

		return f(msg)

	}
}
