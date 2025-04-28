import socket
import json
import time
import threading
import curses
import sys
import random
import struct
import uuid

# Game constants
USE_BINARY_PROTOCOL = True  # Set to False to use JSON protocol

# Message types for binary protocol
MSG_JOIN = 1
MSG_JOIN_ACK = 2
MSG_INPUT = 3
MSG_STATE_UPDATE = 4
MSG_HEARTBEAT = 5

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

class SnakeClient:
    def __init__(self, server_host='127.0.0.1', server_port=25566, use_binary=USE_BINARY_PROTOCOL):
        self.server_host = server_host
        self.server_port = server_port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.player_id = None
        self.player_uuid_bytes = None
        self.grid_size = None
        self.game_state = None
        self.running = False
        self.lock = threading.RLock()
        self.use_binary = use_binary
        
    def join_game(self):
        """Send join request to server"""
        if self.use_binary:
            # Binary join message: [1 byte: message type]
            join_message = struct.pack('!B', MSG_JOIN)
        else:
            # JSON join message
            join_message = json.dumps({'type': 'join'}).encode('utf-8')
        
        # Send join message
        self.socket.sendto(join_message, (self.server_host, self.server_port))
        
        # Wait for acknowledgement
        try:
            data, _ = self.socket.recvfrom(1024)
            
            if self.use_binary:
                # Binary format: [1 byte: message type] [16 bytes: UUID] [2 bytes: width] [2 bytes: height]
                if len(data) >= 21 and data[0] == MSG_JOIN_ACK:
                    # Extract UUID, width, height
                    self.player_uuid_bytes = data[1:17]
                    self.player_id = str(uuid.UUID(bytes=self.player_uuid_bytes))
                    width, height = struct.unpack('!HH', data[17:21])
                    self.grid_size = (width, height)
                    return True
            else:
                # JSON format
                response = json.loads(data.decode('utf-8'))
                if response.get('type') == 'join_ack':
                    self.player_id = response.get('player_id')
                    self.player_uuid_bytes = uuid.UUID(self.player_id).bytes if self.player_id else None
                    self.grid_size = response.get('grid_size')
                    return True
        except Exception as e:
            print(f"Error joining game: {e}")
            
        return False
    
    def send_input(self, direction):
        """Send input to server"""
        if not self.player_id:
            return
        
        if self.use_binary:
            # Binary format: [1 byte: message type] [16 bytes: UUID] [1 byte: direction]
            dir_code = DIR_MAP.get(direction, 0)
            message = struct.pack('!B16sB', MSG_INPUT, self.player_uuid_bytes, dir_code)
        else:
            # JSON format
            message = json.dumps({
                'type': 'input',
                'player_id': self.player_id,
                'direction': direction
            }).encode('utf-8')
        
        self.socket.sendto(message, (self.server_host, self.server_port))
    
    def send_heartbeat(self):
        """Send heartbeat to server to keep connection alive"""
        if not self.player_id:
            return
        
        if self.use_binary:
            # Binary format: [1 byte: message type] [16 bytes: UUID]
            message = struct.pack('!B16s', MSG_HEARTBEAT, self.player_uuid_bytes)
        else:
            # JSON format
            message = json.dumps({
                'type': 'heartbeat',
                'player_id': self.player_id
            }).encode('utf-8')
        
        self.socket.sendto(message, (self.server_host, self.server_port))
    
    def parse_binary_game_state(self, data):
        """Parse binary game state data"""
        if len(data) < 7 or data[0] != MSG_STATE_UPDATE:
            return None
        
        try:
            # Parse header: [1 byte: message type] [4 bytes: timestamp] [1 byte: players count] [1 byte: food count]
            offset = 1
            timestamp, player_count, food_count = struct.unpack('!IBB', data[offset:offset+6])
            offset += 6
            
            # Parse food positions: [food_count * 2 shorts: food coordinates]
            food = []
            for _ in range(food_count):
                if offset + 4 > len(data):
                    break
                x, y = struct.unpack('!HH', data[offset:offset+4])
                food.append((x, y))
                offset += 4
            
            # Parse players
            players = {}
            for _ in range(player_count):
                if offset + 18 > len(data):
                    break
                
                # Parse player header: [16 bytes: UUID] [1 byte: direction] [1 byte: snake length]
                uuid_bytes = data[offset:offset+16]
                player_id = str(uuid.UUID(bytes=uuid_bytes))
                dir_code, snake_length = struct.unpack('!BB', data[offset+16:offset+18])
                offset += 18
                
                # Get direction string
                direction = DIR_MAP_REVERSE.get(dir_code, 'UP')
                
                # Parse snake body segments
                snake_body = []
                for _ in range(snake_length):
                    if offset + 4 > len(data):
                        break
                    x, y = struct.unpack('!HH', data[offset:offset+4])
                    snake_body.append((x, y))
                    offset += 4
                
                # Add player to state
                players[player_id] = {
                    'body': snake_body,
                    'direction': direction
                }
            
            # Assemble game state
            game_state = {
                'timestamp': timestamp,
                'players': players,
                'food': food
            }
            
            return game_state
        
        except Exception as e:
            print(f"Error parsing binary state: {e}")
            return None
    
    def listen_for_updates(self):
        """Listen for game state updates from server"""
        self.socket.settimeout(0.1)  # Small timeout to allow checking if running
        
        while self.running:
            try:
                data, _ = self.socket.recvfrom(4096)  # Larger buffer for game state
                
                if self.use_binary:
                    # Binary format
                    if data and data[0] == MSG_STATE_UPDATE:
                        with self.lock:
                            parsed_state = self.parse_binary_game_state(data)
                            if parsed_state:
                                self.game_state = parsed_state
                else:
                    # JSON format
                    message = json.loads(data.decode('utf-8'))
                    if message.get('type') == 'state_update':
                        with self.lock:
                            self.game_state = message.get('state')
                            
            except socket.timeout:
                continue
            except Exception as e:
                print(f"Error receiving update: {e}")
    
    def heartbeat_loop(self):
        """Periodically send heartbeats to server"""
        while self.running:
            self.send_heartbeat()
            time.sleep(2)  # Send heartbeat every 2 seconds
    
    def start(self):
        """Start the client"""
        if not self.join_game():
            print("Failed to join game")
            return
            
        print(f"Joined game as player {self.player_id}")
        
        self.running = True
        
        # Start listener thread
        listener_thread = threading.Thread(target=self.listen_for_updates)
        listener_thread.daemon = True
        listener_thread.start()
        
        # Start heartbeat thread
        heartbeat_thread = threading.Thread(target=self.heartbeat_loop)
        heartbeat_thread.daemon = True
        heartbeat_thread.start()
        
        # Start the UI
        try:
            curses.wrapper(self.run_game_ui)
        except KeyboardInterrupt:
            pass
        finally:
            self.running = False
    
    def run_game_ui(self, stdscr):
        """Run the game UI using curses"""
        # Set up curses
        curses.curs_set(0)  # Hide cursor
        stdscr.nodelay(1)   # Non-blocking input
        stdscr.timeout(100) # UI refresh rate
        
        # Initialize colors
        curses.start_color()
        curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)  # Snake color
        curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)    # Food color
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK) # Other snakes
        
        # Game loop
        while self.running:
            # Handle input
            try:
                key = stdscr.getch()
                if key == ord('q'):
                    self.running = False
                    break
                elif key == curses.KEY_UP:
                    self.send_input('UP')
                elif key == curses.KEY_RIGHT:
                    self.send_input('RIGHT')
                elif key == curses.KEY_DOWN:
                    self.send_input('DOWN')
                elif key == curses.KEY_LEFT:
                    self.send_input('LEFT')
            except:
                pass
            
            # Clear screen
            stdscr.clear()
            
            # Draw game state
            with self.lock:
                if self.game_state:
                    self.draw_game_state(stdscr)
            
            # Draw instructions
            max_y, max_x = stdscr.getmaxyx()
            protocol = "Binary" if self.use_binary else "JSON"
            instructions = f"Use arrow keys to move. Press 'q' to quit. Protocol: {protocol}"
            stdscr.addstr(max_y-1, 0, instructions)
            
            # Refresh screen
            stdscr.refresh()
    
    def draw_game_state(self, stdscr):
        """Draw the current game state"""
        if not self.game_state or not self.grid_size:
            return
            
        # Draw border
        width, height = self.grid_size
        stdscr.addstr(0, 0, '+' + '-' * width + '+')
        for y in range(height):
            stdscr.addstr(y+1, 0, '|')
            stdscr.addstr(y+1, width+1, '|')
        stdscr.addstr(height+1, 0, '+' + '-' * width + '+')
        
        # Draw food
        for x, y in self.game_state.get('food', []):
            try:
                stdscr.addch(y+1, x+1, 'o', curses.color_pair(2))
            except:
                pass
        
        # Draw players
        for pid, player_data in self.game_state.get('players', {}).items():
            snake_body = player_data.get('body', [])
            
            # Determine color based on whether it's our snake or not
            color = curses.color_pair(1) if pid == self.player_id else curses.color_pair(3)
            
            # Draw head
            if snake_body:
                head_x, head_y = snake_body[0]
                try:
                    stdscr.addch(head_y+1, head_x+1, 'O', color)
                except:
                    pass
                
                # Draw body
                for x, y in snake_body[1:]:
                    try:
                        stdscr.addch(y+1, x+1, 'x', color)
                    except:
                        pass
        
        # Draw score and stats
        if self.player_id in self.game_state.get('players', {}):
            my_snake = self.game_state['players'][self.player_id]
            score = len(my_snake['body']) - 3  # Starting length is 3
            timestamp = self.game_state.get('timestamp', 0)
            stdscr.addstr(height+2, 0, f"Score: {score} | Players: {len(self.game_state['players'])} | Tick: {timestamp}")
        else:
            stdscr.addstr(height+2, 0, "You are dead! Restart to play again.")

def main():
    # Parse command line arguments for server address and protocol
    server_host = '127.0.0.1'
    server_port = 25566
    use_binary = USE_BINARY_PROTOCOL
    
    if len(sys.argv) > 1:
        server_host = sys.argv[1]
    if len(sys.argv) > 2:
        server_port = int(sys.argv[2])
    if len(sys.argv) > 3:
        use_binary = sys.argv[3].lower() in ('true', 't', 'yes', 'y', '1')
    
    # Create and start client
    client = SnakeClient(server_host, server_port, use_binary)
    client.start()

if __name__ == '__main__':
    main() 