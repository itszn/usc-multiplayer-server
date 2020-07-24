package main

import "encoding/json"

type Json map[string]interface{}

func JsonEncodeBytes(data Json) ([]byte, error) {
	return json.Marshal(data)
}

func JsonEncode(data Json) (string, error) {
	res, err := json.Marshal(data)
	return string(res), err
}

func JsonDecode(data string) (Json, error) {
	return JsonDecodeBytes([]byte(data))
}

func JsonDecodeBytes(data []byte) (Json, error) {
	var res Json
	err := json.Unmarshal(data, &res)
	if err != nil {
		return make(Json), err
	}
	return res, err
}

// Make sure we get an int back from the json number
func Json_int(val interface{}) int {
	switch val.(type) {
	case int:
		return val.(int)
	case float64:
		return int(val.(float64))
	default:
		panic("Not a number")
	}
}

// Make sure we get an int back from the json number
func Json_float(val interface{}) float64 {
	switch val.(type) {
	case int:
		return float64(val.(int))
	case float64:
		return val.(float64)
	default:
		panic("Not a number")
	}
}
