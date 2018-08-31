package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net"
	"os"
	"strings"

	"github.com/vmihailenco/msgpack"
)

type Header struct {
	length  uint32
	typcode uint32
}

type MsgInsert struct {
	header Header
	length uint32
	// data   []byte
	// data interface{}
}

type MsgDelete struct {
	header Header
	// data   []byte/
	// data interface{}
}

type MsgShutdown struct {
	header Header
	data   []byte
}
type MsgReply struct {
	header     Header
	returnCode uint32
	numReturn  uint32
	data       []byte
}
type InputBuffer struct {
	buffer string
}

func (b *InputBuffer) input() {
	reader := bufio.NewReader(os.Stdin)
	// b.buffer, _ = reader.ReadString('\n')
	buffer, _ := reader.ReadString('\n')
	b.buffer = strings.TrimSuffix(buffer, "\n")
	fmt.Print(b.buffer)
}

const (
	Address = "localhost:1090"
)

type DB struct {
	conn    net.Conn
	address string
}

func error_handle(err error) {
	if err != nil {
		fmt.Print(err)
	}
}

func Connect(address string) DB {
	conn, err := net.Dial("tcp", address)
	error_handle(err)
	return DB{conn, address}
}

func (db *DB) Close() {
	db.conn.Close()
}

func (db *DB) Put(s string) {
	var raw map[string]interface{}
	json.Unmarshal([]byte(s), &raw)
	out, err := json.Marshal(raw)
	if err != nil {
		fmt.Println("Json format, please expect input")
	}
	if raw["_id"] != nil {
		println(string(out))
	}
	pack, err := msgpack.Marshal(raw)
	fmt.Print(pack)
}

func main() {
	db := Connect(Address)
	// 82 a3 5f 69 64 a3 6b 65 79 a5 68 65 6c 6c 6f a5 77 6f 72 6c 64
	db.Put(`{ "_id":"key" ,"hello": "world"`)
	db.Close()
}
