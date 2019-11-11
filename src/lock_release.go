// +build !debug

package main

import (
	"sync"
)


type Lock struct {
	sync.RWMutex
}

func (self *Lock) init(mid int) {}
