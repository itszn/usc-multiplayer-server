# USC Multiplayer TCP protocol

USC Multiplayer uses a TCP connection between the client and the server.
The tcp protocol is built to allow multiple types of packets, however currently only JSON packets are used.

Each packet starts with byte indicating the packet type. This is followed by the packet content of the selected type

This packet type byte is one value from the following enum:

```c++
enum TCPPacketMode
{
	NOT_READING = 0,
	JSON_LINE,
	UNKNOWN
};
```

Currently only `JSON_LINE` is supported by the server. The enum may expand as new modes are added.


### JSON_LINE

This packet type encodes JSON data, delineated with a new line character.
Here is the packet setup (including the packet type byte):

```
[ "\x01" ][ JSON ascii data ][ "\n" ]
```





