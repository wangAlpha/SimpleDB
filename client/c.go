package main

import "encoding/json"

func main() {
    in := []byte(`{ "votes": { "option_A": "3" } }`)
    var raw map[string]interface{}
    json.Unmarshal(in, &raw)
    raw["count"] = 1
    out, _ := json.Marshal(raw)
    println(string(out))
}
