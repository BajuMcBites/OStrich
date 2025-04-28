# Snake Game Server

This is a Python implementation of a multiplayer snake game server that works with the compatible client.

## Requirements

- Python 3.6+
- Socket support (standard library)

## Starting the Server

To start the server, run:

```bash
python3 snake_server.py [port]
```

The default port is 25566 if not specified.

## Protocol Details

The server and client communicate using a binary protocol:

### Message Types
- JOIN (1): Client requests to join the game
- JOIN_ACK (2): Server acknowledges join request and assigns UUID
- INPUT (3): Client sends direction change
- STATE_UPDATE (4): Server broadcasts game state
- HEARTBEAT (5): Client indicates it's still active

### Binary Format
Different message types have different formats:

#### JOIN
Client → Server: `[1 byte: MSG_JOIN]`

#### JOIN_ACK
Server → Client: `[1 byte: MSG_JOIN_ACK][16 bytes: UUID][2 bytes: width][2 bytes: height]`

#### INPUT
Client → Server: `[1 byte: MSG_INPUT][16 bytes: UUID][1 byte: direction]`

#### STATE_UPDATE
Server → Client: 
```
[1 byte: MSG_STATE_UPDATE]
[4 bytes: timestamp]
[1 byte: player count]
[1 byte: food count]
[food_count * 4 bytes: food coordinates (x,y shorts)]
For each player:
  [16 bytes: UUID]
  [1 byte: direction]
  [1 byte: snake length]
  [snake_length * 4 bytes: snake body coordinates (x,y shorts)]
```

#### HEARTBEAT
Client → Server: `[1 byte: MSG_HEARTBEAT][16 bytes: UUID]`

## Game Details

- Grid size: 40x30
- Game tick rate: 5 updates per second
- Food count: 5 items at all times
- Snake starting length: 3 segments
- Snake collision results in removal from game
- Wraparound grid (snakes can go through walls) 