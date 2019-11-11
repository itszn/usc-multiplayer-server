// +build debug

package main

import (
	"fmt"
	"sync"
	"runtime"
	"bytes"
	"strconv"
	"runtime/debug"
)

var (
	G_THREADS = make(map[uint64]map[int]int)
)

func getGID() uint64 {
    b := make([]byte, 64)
    b = b[:runtime.Stack(b, false)]
    b = bytes.TrimPrefix(b, []byte("goroutine "))
    b = b[:bytes.IndexByte(b, ' ')]
    n, _ := strconv.ParseUint(string(b), 10, 64)
    return n
}


type Lock struct {
	mtx sync.RWMutex
	mid int
}

func (self *Lock) init(mid int) {
	fmt.Println("Debugging lock for lock type %u", mid)
	self.mid = mid
}

func (self *Lock) checkLock() {
	gid := getGID()
	tinfo := G_THREADS[gid]
	if tinfo == nil {
		G_THREADS[gid] = make(map[int]int)
		tinfo = G_THREADS[gid]
	}
	for l,v := range tinfo {
		if v>0 && l > self.mid {
			fmt.Printf("Lock %d locked inside %d\n",self.mid,l)
			debug.PrintStack()
			break
		}
	}
	if v, ok := tinfo[self.mid]; ok {
		G_THREADS[gid][self.mid] = v+1
	} else {
		G_THREADS[gid][self.mid] = 1
	}

}

func (self *Lock) Lock() {
	self.mtx.Lock()
	self.checkLock()
}

func (self *Lock) Unlock() {
	G_THREADS[getGID()][self.mid] -= 1
	self.mtx.Unlock()
}

func (self *Lock) RLock() {
	self.mtx.RLock()
	self.checkLock()
}
func (self *Lock) RUnlock() {
	G_THREADS[getGID()][self.mid] -= 1
	self.mtx.RUnlock()
}
