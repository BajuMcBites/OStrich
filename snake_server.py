import socket
import time
import random
import json
import threading
import uuid
import struct
from typing import Dict, List, Tuple, Set

# Game constants
GRID_WIDTH = 40
GRID_HEIGHT = 30
TICK_RATE = 5  # Game updates per second
FOOD_COUNT = 5
MAX_SNAKE_LENGTH = 100  # Maximum snake length for binary format
USE_BINARY_PROTOCOL = True  # Set to False to use JSON protocol

# Message types for binary protocol
MSG_JOIN = 1
MSG_JOIN_ACK = 2
MSG_INPUT = 3
MSG_STATE_UPDATE = 4
MSG_HEARTBEAT = 5

# Direction vectors (up, right, down, left)
DIRECTIONS = {
    'UP': (0, -1),
    'RIGHT': (1, 0),
    'DOWN': (0, 1),
    'LEFT': (-1, 0)
}

# Direction constants for binary protocol
DIR_UP = 0
DIR_RIGHT = 1
DIR_DOWN = 2
DIR_LEFT = 3

# Direction mapping for binary protocol
DIR_MAP = {
    'UP': DIR_UP,
    'RIGHT': DIR_RIGHT,
    'DOWN': DIR_DOWN,
    'LEFT': DIR_LEFT
}

DIR_MAP_REVERSE = {
    DIR_UP: 'UP',
    DIR_RIGHT: 'RIGHT',
    DIR_DOWN: 'DOWN',
    DIR_LEFT: 'LEFT'
}

# Binary protocol format:
# State update:
# [1 byte: message type] [4 bytes: timestamp] [1 byte: players count] [1 byte: food count]
# [food_count * 2 shorts: food coordinates]
# [For each player: [16 bytes: player UUID] [1 byte: direction] [1 byte: snake length] [snake_length * 2 shorts: snake body]]

class SnakeGame:
    def __init__(self):
        self.players = {}  # player_id -> Snake object
        self.food = set()  # Set of food coordinates
        self.lock = threading.RLock()  # Lock for thread safety
        self.running = False
        self.timestamp = 0  # Game clock for synchronization
        
        # Initialize food
        self.regenerate_food()
    
    def regenerate_food(self):
        """Ensure we have the correct amount of food on the board"""
        with self.lock:
            # Calculate how many new food items we need
            needed = FOOD_COUNT - len(self.food)
            
            if needed <= 0:
                return
                
            # Get all occupied positions
            occupied = set()
            for player_id, snake in self.players.items():
                occupied.update(snake.body)
            occupied.update(self.food)
            
            # Generate new food
            for _ in range(needed):
                while True:
                    pos = (random.randint(0, GRID_WIDTH-1), random.randint(0, GRID_HEIGHT-1))
                    if pos not in occupied:
                        self.food.add(pos)
                        occupied.add(pos)
                        break
    
    def register_player(self, player_id, client_addr):
        """Register a new player"""
        with self.lock:
            # Create snake in a random position
            while True:
                head_x = random.randint(5, GRID_WIDTH-5)
                head_y = random.randint(5, GRID_HEIGHT-5)
                head = (head_x, head_y)
                
                # Check if position is valid
                valid = True
                for pid, snake in self.players.items():
                    if head in snake.body:
                        valid = False
                        break
                
                if valid and head not in self.food:
                    break
            
            # Random direction
            direction = random.choice(list(DIRECTIONS.keys()))
            
            # Create snake with 3 segments
            snake = Snake(head, direction)
            
            # Add player to game
            self.players[player_id] = snake
            
            return player_id
    
    def remove_player(self, player_id):
        """Remove a player from the game"""
        with self.lock:
            if player_id in self.players:
                del self.players[player_id]
    
    def update_player_direction(self, player_id, direction):
        """Update a player's snake direction"""
        with self.lock:
            if player_id in self.players and direction in DIRECTIONS:
                # Prevent 180-degree turns
                snake = self.players[player_id]
                curr_dir_vec = DIRECTIONS[snake.direction]
                new_dir_vec = DIRECTIONS[direction]
                
                # Can't reverse direction
                if (curr_dir_vec[0] + new_dir_vec[0] == 0 and 
                    curr_dir_vec[1] + new_dir_vec[1] == 0):
                    return
                
                snake.direction = direction
    
    def update_game_state(self):
        """Update the game state for one tick"""
        with self.lock:
            self.timestamp += 1
            
            # Move each snake
            dead_players = []
            for player_id, snake in self.players.items():
                # Move the snake
                direction_vector = DIRECTIONS[snake.direction]
                head_x, head_y = snake.body[0]
                new_head = ((head_x + direction_vector[0]) % GRID_WIDTH, 
                            (head_y + direction_vector[1]) % GRID_HEIGHT)
                
                # Check if new head position hits its own body
                if new_head in snake.body[:-1]:
                    dead_players.append(player_id)
                    continue
                
                # Check if new head position hits another snake
                collision = False
                for pid, other_snake in self.players.items():
                    if pid == player_id:
                        continue
                    if new_head in other_snake.body:
                        collision = True
                        break
                
                if collision:
                    dead_players.append(player_id)
                    continue
                
                # Move the snake
                snake.body.insert(0, new_head)
                
                # Check if snake ate food
                if new_head in self.food:
                    self.food.remove(new_head)
                    # snake grows, don't remove tail
                else:
                    # remove tail
                    snake.body.pop()
            
            # Remove dead players
            for player_id in dead_players:
                del self.players[player_id]
            
            # Regenerate food if needed
            self.regenerate_food()
    
    def get_game_state(self):
        """Get the current game state as a serializable object"""
        with self.lock:
            state = {
                'timestamp': self.timestamp,
                'players': {},
                'food': list(self.food)
            }
            
            for player_id, snake in self.players.items():
                state['players'][player_id] = {
                    'body': snake.body,
                    'direction': snake.direction
                }
            
            return state
    
    def get_binary_game_state(self):
        """Get the current game state as binary data"""
        with self.lock:
            # Format: [1 byte: message type] [4 bytes: timestamp] [1 byte: players count] [1 byte: food count]
            # [food_count * 2 shorts: food coordinates]
            # [For each player: [16 bytes: player UUID] [1 byte: direction] [1 byte: snake length] [snake_length * 2 shorts: snake body]]
            
            # Calculate binary size
            food_bytes = len(self.food) * 4  # 2 shorts (2 bytes each) per food
            player_bytes = sum(min(len(snake.body), MAX_SNAKE_LENGTH) * 4 + 18 for snake in self.players.values())
            
            # Pack header
            data = struct.pack(
                '!BIBB',  # !: network byte order, B: unsigned char, I: unsigned int
                MSG_STATE_UPDATE,
                self.timestamp,
                len(self.players),
                len(self.food)
            )
            
            # Pack food
            for x, y in self.food:
                data += struct.pack('!HH', x, y)  # H: unsigned short
            
            # Pack players
            for player_id, snake in self.players.items():
                # Convert UUID string to bytes (16 bytes)
                uuid_bytes = uuid.UUID(player_id).bytes
                
                # Get direction code
                dir_code = DIR_MAP.get(snake.direction, DIR_UP)
                
                # Limit snake length to MAX_SNAKE_LENGTH
                snake_body = snake.body[:MAX_SNAKE_LENGTH]
                
                # Pack player header
                data += struct.pack(
                    '!16sBB',
                    uuid_bytes,
                    dir_code,
                    len(snake_body)
                )
                
                # Pack snake body
                for x, y in snake_body:
                    data += struct.pack('!HH', x, y)
            
            return data


class Snake:
    def __init__(self, head, direction):
        # Create a snake with 3 segments
        self.direction = direction
        dir_vec = DIRECTIONS[direction]
        
        # Calculate body positions (going backwards from head)
        self.body = [
            head,
            ((head[0] - dir_vec[0]) % GRID_WIDTH, (head[1] - dir_vec[1]) % GRID_HEIGHT),
            ((head[0] - 2*dir_vec[0]) % GRID_WIDTH, (head[1] - 2*dir_vec[1]) % GRID_HEIGHT)
        ]


class SnakeServer:
    def __init__(self, host='0.0.0.0', port=25566, use_binary=USE_BINARY_PROTOCOL):
        self.host = host
        self.port = port
        self.game = SnakeGame()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.clients = {}  # player_id -> (addr, last_seen_time)
        self.running = False
        self.use_binary = use_binary
        
    def start(self):
        """Start the server"""
        self.socket.bind((self.host, self.port))
        self.running = True
        self.game.running = True
        
        # Start game update thread
        game_thread = threading.Thread(target=self.game_loop)
        game_thread.daemon = True
        game_thread.start()
        
        # Start client timeout checker
        timeout_thread = threading.Thread(target=self.check_client_timeouts)
        timeout_thread.daemon = True
        timeout_thread.start()
        
        print(f"Snake server running on {self.host}:{self.port}")
        
        try:
            self.listen_for_clients()
        except KeyboardInterrupt:
            print("Server shutting down...")
        finally:
            self.running = False
            self.game.running = False
            self.socket.close()
    
    def listen_for_clients(self):
        """Listen for client messages"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(1024)
                self.handle_client_message(data, addr)
            except Exception as e:
                print(f"Error handling client message: {e}")
    
    def handle_client_message(self, data, addr):
        """Process a message from a client"""
        if self.use_binary:
            self.handle_binary_message(data, addr)
        else:
            self.handle_json_message(data, addr)
    
    def handle_json_message(self, data, addr):
        """Process a JSON message from a client"""
        try:
            # Decode message
            message = json.loads(data.decode('utf-8'))
            message_type = message.get('type')
            
            if message_type == 'join':
                # New player joining
                player_id = str(uuid.uuid4())
                self.game.register_player(player_id, addr)
                self.clients[player_id] = (addr, time.time())
                
                # Send player ID back to client
                response = {
                    'type': 'join_ack',
                    'player_id': player_id,
                    'grid_size': (GRID_WIDTH, GRID_HEIGHT)
                }
                self.socket.sendto(json.dumps(response).encode('utf-8'), addr)
                
                print(f"New player joined: {player_id} from {addr}")
            
            elif message_type == 'input':
                # Player sending input (direction change)
                player_id = message.get('player_id')
                direction = message.get('direction')
                
                if player_id in self.clients:
                    # Update last seen time
                    self.clients[player_id] = (addr, time.time())
                    
                    # Update player direction
                    self.game.update_player_direction(player_id, direction)
            
            elif message_type == 'heartbeat':
                # Client heartbeat to keep connection alive
                player_id = message.get('player_id')
                if player_id in self.clients:
                    # Update last seen time
                    self.clients[player_id] = (addr, time.time())
                    
                    # Send current game state
                    self.send_game_state_to_client(player_id)
        
        except json.JSONDecodeError:
            print(f"Invalid JSON from {addr}")
        except Exception as e:
            print(f"Error processing message from {addr}: {e}")
    
    def handle_binary_message(self, data, addr):
        """Process a binary message from a client"""
        try:
            if len(data) < 1:
                return
                
            # First byte is message type
            msg_type = data[0]
            
            if msg_type == MSG_JOIN:
                # New player joining
                player_id = str(uuid.uuid4())
                self.game.register_player(player_id, addr)
                self.clients[player_id] = (addr, time.time())
                
                # Send acknowledgment (MSG_JOIN_ACK + 16-byte uuid + 2-byte width + 2-byte height)
                uuid_bytes = uuid.UUID(player_id).bytes
                response = struct.pack('!B16sHH', MSG_JOIN_ACK, uuid_bytes, GRID_WIDTH, GRID_HEIGHT)
                self.socket.sendto(response, addr)
                
                print(f"New player joined: {player_id} from {addr}")
                
            elif msg_type == MSG_INPUT:
                # Format: [1 byte: message type] [16 bytes: player UUID] [1 byte: direction]
                if len(data) < 18:
                    return
                    
                # Extract UUID
                uuid_bytes = data[1:17]
                player_id = str(uuid.UUID(bytes=uuid_bytes))
                
                # Extract direction
                dir_code = data[17]
                if dir_code in DIR_MAP_REVERSE:
                    direction = DIR_MAP_REVERSE[dir_code]
                    
                    if player_id in self.clients:
                        # Update last seen time
                        self.clients[player_id] = (addr, time.time())
                        
                        # Update player direction
                        self.game.update_player_direction(player_id, direction)
                        
            elif msg_type == MSG_HEARTBEAT:
                # Format: [1 byte: message type] [16 bytes: player UUID]
                if len(data) < 17:
                    return
                    
                # Extract UUID
                uuid_bytes = data[1:17]
                player_id = str(uuid.UUID(bytes=uuid_bytes))
                
                if player_id in self.clients:
                    # Update last seen time
                    self.clients[player_id] = (addr, time.time())
                    
                    # Send current game state
                    self.send_game_state_to_client(player_id)
                
        except Exception as e:
            print(f"Error processing binary message from {addr}: {e}")
    
    def game_loop(self):
        """Game update loop"""
        while self.game.running:
            try:
                # Update game state
                self.game.update_game_state()
                
                # Send updated state to all clients
                self.broadcast_game_state()
                
                # Sleep to maintain tick rate
                time.sleep(1 / TICK_RATE)
            
            except Exception as e:
                print(f"Error in game loop: {e}")
    
    def broadcast_game_state(self):
        """Send game state to all connected clients"""
        if self.use_binary:
            # Binary format
            binary_state = self.game.get_binary_game_state()
            
            # Send to all clients
            for player_id, (addr, _) in list(self.clients.items()):
                try:
                    self.socket.sendto(binary_state, addr)
                except:
                    print(f"Failed to send to client {player_id}")
        else:
            # JSON format
            game_state = self.game.get_game_state()
            state_message = {
                'type': 'state_update',
                'state': game_state
            }
            
            encoded_message = json.dumps(state_message).encode('utf-8')
            
            # Send to all clients
            for player_id, (addr, _) in list(self.clients.items()):
                try:
                    self.socket.sendto(encoded_message, addr)
                except:
                    print(f"Failed to send to client {player_id}")
    
    def send_game_state_to_client(self, player_id):
        """Send current game state to a specific client"""
        if player_id in self.clients:
            addr = self.clients[player_id][0]
            
            if self.use_binary:
                # Binary format
                binary_state = self.game.get_binary_game_state()
                try:
                    self.socket.sendto(binary_state, addr)
                except:
                    print(f"Failed to send state to client {player_id}")
            else:
                # JSON format
                game_state = self.game.get_game_state()
                state_message = {
                    'type': 'state_update',
                    'state': game_state
                }
                
                try:
                    self.socket.sendto(json.dumps(state_message).encode('utf-8'), addr)
                except:
                    print(f"Failed to send state to client {player_id}")
    
    def check_client_timeouts(self):
        """Remove clients that haven't sent messages in a while"""
        TIMEOUT = 10  # seconds
        
        while self.running:
            current_time = time.time()
            
            # Find timed out clients
            timed_out = []
            for player_id, (_, last_seen) in self.clients.items():
                if current_time - last_seen > TIMEOUT:
                    timed_out.append(player_id)
            
            # Remove timed out clients
            for player_id in timed_out:
                print(f"Client {player_id} timed out")
                if player_id in self.clients:
                    del self.clients[player_id]
                self.game.remove_player(player_id)
            
            time.sleep(1)


if __name__ == '__main__':
    server = SnakeServer(host='0.0.0.0', port=25566, use_binary=USE_BINARY_PROTOCOL)
    server.start()
