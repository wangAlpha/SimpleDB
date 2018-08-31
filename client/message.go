package main

type myStruct struct {
	ID   uint32
	Data uint32
}

// func main() {
// 	var bin_buf bytes.Buffer
// 	x := &MsgInsert{header: {0, 2}}
// 	// binary.Write(&bin_buf, binary.BigEndian, x)
// 	// fmt.Printf("% x", sha1.Sum(bin_buf.Bytes()))
// 	bytesSlice := *(*[]byte)(unsafe.Pointer(x))
// 	fmt.Printf("% x", bin_buf.Bytes())
// }
