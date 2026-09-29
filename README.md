# socket-message-demo

Two C processes, a client and a server, exchange messages over a TCP socket on `127.0.0.1:5555`. Each message holds a float value and a string label.

## Build and run

```bash
make
./server        # terminal 1
./client        # terminal 2
```

The client sends three messages. The server replies to each with the value doubled and `"doubled "` prefixed to the label.

## Wire format

| Field | Type | Notes |
|---|---|---|
| value | `uint32` | IEEE-754 bits of the float, network byte order |
| len | `uint8` | label length (max 255) |
| label | `char[len]` | no trailing NUL |
