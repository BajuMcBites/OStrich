import socket
import time
import random
import json
import threading
import uuid
import struct
import os
from typing import Dict, List, Tuple, Set
import binascii

# Game constants
GRID_WIDTH = 680
GRID_HEIGHT = 480
TICK_RATE = 20  # Game updates per second
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
            # Create snake with 3 segments
            snake = Snake(0, 0)
            
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
            # Pack header
            data = struct.pack(
                '!BIB',  # !: network byte order, B: unsigned char, I: unsigned int
                MSG_STATE_UPDATE,
                self.timestamp,
                len(self.players)
            )
            
            for player_id, snake in self.players.items():
                # Pack player header
                data += struct.pack(
                    '8shh',
                    binascii.a2b_hex(player_id.encode('ascii')),
                    snake.x,
                    snake.y
                )
                
            return data


class Snake:
    def __init__(self, x, y):
        # Create a snake with 3 segments
        self.x = x
        self.y = y


class SnakeServer:
    def __init__(self, port=25566, use_binary=USE_BINARY_PROTOCOL):
        self.host = '192.168.208.75'
        self.port = port
        self.game = SnakeGame()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.clients = {}  # player_id -> (addr, last_seen_time)
        self.running = False
        self.use_binary = use_binary
        
    def start(self):
        """Start the server"""
        self.socket.bind(('192.168.208.75', self.port))
        print(socket.gethostbyaddr(socket.gethostname()))
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
    
    def handle_binary_message(self, data, addr):
        """Process a binary message from a client"""
        try:
            if len(data) < 1:
                return
            
            # First byte is message type
            msg_type = data[0]
            
            if msg_type == MSG_JOIN:
                # New player joining
                uuid = os.urandom(8)
                player_id = binascii.hexlify(uuid).decode('ascii')

                self.game.register_player(player_id, addr)
                self.clients[player_id] = (addr, time.time())
                # Send acknowledgment (MSG_JOIN_ACK + 8-byte uuid + 2-byte width + 2-byte height)

                response = struct.pack('!B8s', MSG_JOIN_ACK, uuid)
                self.socket.sendto(response, addr)
                
                print(f"New player joined: {player_id} from {addr}")
                
            elif msg_type == MSG_INPUT:
                # Format: [1 byte: message type] [8 bytes: player UUID] [2 byte: x position] [2 byte: y position]
                if len(data) < 13:
                    return
                # Extract UUID
                uuid_bytes = data[1:9]
                player_id = binascii.hexlify(uuid_bytes).decode('ascii')

                if player_id in self.clients:
                        # Extract direction
                # dir_code = data[17]
                    x = int.from_bytes(data[9:11], byteorder='little')
                    y = int.from_bytes(data[11:13], byteorder='little')
                    self.clients[player_id] = (addr, time.time())

                    self.game.players[player_id].x = x
                    self.game.players[player_id].y = y
                
        except Exception as e:
            print(f"Error processing binary message from {addr}: {e}")
    
    def game_loop(self):
        """Game update loop"""
        while self.running:
            try:
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
    server = SnakeServer(port=25566, use_binary=USE_BINARY_PROTOCOL)
    server.start()
