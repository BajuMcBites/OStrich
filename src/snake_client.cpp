#include <arpa/inet.h>
#include <fcntl.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Game constants
#define USE_BINARY_PROTOCOL true

// Message types for binary protocol
#define MSG_JOIN 1
#define MSG_JOIN_ACK 2
#define MSG_INPUT 3
#define MSG_STATE_UPDATE 4
#define MSG_HEARTBEAT 5

// Direction constants for binary protocol
#define DIR_UP 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_LEFT 3

// Snake game state
struct Position {
    uint16_t x;
    uint16_t y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct Snake {
    std::vector<Position> body;
    std::string direction;
};

struct GameState {
    uint32_t timestamp;
    std::map<std::string, Snake> players;
    std::vector<Position> food;
};

class SnakeClient {
   private:
    std::string server_host;
    int server_port;
    int socket_fd;
    struct sockaddr_in server_addr;

    std::string player_id;
    uuid_t player_uuid;
    bool has_uuid;
    std::pair<int, int> grid_size;

    GameState game_state;
    bool running;
    std::mutex game_state_mutex;

    // Network buffer
    static const int BUFFER_SIZE = 8192;
    uint8_t buffer[BUFFER_SIZE];

    // Convert direction string to direction code
    uint8_t direction_to_code(const std::string& direction) {
        if (direction == "UP") return DIR_UP;
        if (direction == "RIGHT") return DIR_RIGHT;
        if (direction == "DOWN") return DIR_DOWN;
        if (direction == "LEFT") return DIR_LEFT;
        return DIR_UP;  // Default
    }

    // Convert direction code to direction string
    std::string code_to_direction(uint8_t code) {
        switch (code) {
            case DIR_UP:
                return "UP";
            case DIR_RIGHT:
                return "RIGHT";
            case DIR_DOWN:
                return "DOWN";
            case DIR_LEFT:
                return "LEFT";
            default:
                return "UP";
        }
    }

    // Network byte order helpers
    uint16_t htons_custom(uint16_t value) {
        return htons(value);
    }

    uint32_t htonl_custom(uint32_t value) {
        return htonl(value);
    }

    uint16_t ntohs_custom(uint16_t value) {
        return ntohs(value);
    }

    uint32_t ntohl_custom(uint32_t value) {
        return ntohl(value);
    }

   public:
    SnakeClient(const std::string& host = "127.0.0.1", int port = 25566)
        : server_host(host), server_port(port), running(false), has_uuid(false) {
        // Initialize socket
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            exit(1);
        }

        // Set up server address
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);

        if (inet_pton(AF_INET, server_host.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << server_host << std::endl;
            exit(1);
        }

        // Set socket to non-blocking mode
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    }

    ~SnakeClient() {
        close(socket_fd);
    }

    bool join_game() {
        // Prepare binary join message
        buffer[0] = MSG_JOIN;

        // Send join message
        sendto(socket_fd, buffer, 1, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Wait for acknowledgement (with timeout)
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {
            socklen_t server_len = sizeof(server_addr);
            int nbytes = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr,
                                  &server_len);

            if (nbytes > 0) {
                // Check if it's a join acknowledgment
                if (buffer[0] == MSG_JOIN_ACK && nbytes >= 21) {
                    // Extract UUID
                    memcpy(player_uuid, buffer + 1, 16);
                    has_uuid = true;

                    // Convert UUID to string
                    char uuid_str[37];
                    uuid_unparse(player_uuid, uuid_str);
                    player_id = uuid_str;

                    // Extract grid size
                    uint16_t width, height;
                    memcpy(&width, buffer + 17, 2);
                    memcpy(&height, buffer + 19, 2);

                    // Convert from network byte order
                    grid_size.first = ntohs_custom(width);
                    grid_size.second = ntohs_custom(height);

                    return true;
                }
            }

            // Sleep a bit before trying again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return false;
    }

    void send_input(const std::string& direction) {
        if (!has_uuid) return;

        // Binary format: [MSG_INPUT(1)] [UUID(16)] [direction(1)]
        buffer[0] = MSG_INPUT;
        memcpy(buffer + 1, player_uuid, 16);
        buffer[17] = direction_to_code(direction);

        sendto(socket_fd, buffer, 18, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }

    void send_heartbeat() {
        if (!has_uuid) return;

        // Binary format: [MSG_HEARTBEAT(1)] [UUID(16)]
        buffer[0] = MSG_HEARTBEAT;
        memcpy(buffer + 1, player_uuid, 16);

        sendto(socket_fd, buffer, 17, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }

    bool parse_binary_game_state(const uint8_t* data, int length) {
        if (length < 7 || data[0] != MSG_STATE_UPDATE) {
            return false;
        }

        try {
            int offset = 1;

            // Parse header: [4 bytes: timestamp] [1 byte: players count] [1 byte: food count]
            uint32_t timestamp;
            uint8_t player_count, food_count;

            memcpy(&timestamp, data + offset, 4);
            timestamp = ntohl_custom(timestamp);
            offset += 4;

            player_count = data[offset++];
            food_count = data[offset++];

            // Create a new game state
            GameState new_state;
            new_state.timestamp = timestamp;

            // Parse food positions
            for (int i = 0; i < food_count; i++) {
                if (offset + 4 > length) break;

                uint16_t x, y;
                memcpy(&x, data + offset, 2);
                memcpy(&y, data + offset + 2, 2);
                offset += 4;

                Position food_pos;
                food_pos.x = ntohs_custom(x);
                food_pos.y = ntohs_custom(y);
                new_state.food.push_back(food_pos);
            }

            // Parse players
            for (int i = 0; i < player_count; i++) {
                if (offset + 18 > length) break;

                // Parse player header: [16 bytes: UUID] [1 byte: direction] [1 byte: body length]
                uuid_t uuid_bytes;
                memcpy(uuid_bytes, data + offset, 16);
                offset += 16;

                char uuid_str[37];
                uuid_unparse(uuid_bytes, uuid_str);
                std::string pid = uuid_str;

                uint8_t dir_code = data[offset++];
                uint8_t snake_length = data[offset++];

                // Get direction string
                std::string direction = code_to_direction(dir_code);

                // Parse snake body
                Snake snake;
                snake.direction = direction;

                for (int j = 0; j < snake_length; j++) {
                    if (offset + 4 > length) break;

                    uint16_t x, y;
                    memcpy(&x, data + offset, 2);
                    memcpy(&y, data + offset + 2, 2);
                    offset += 4;

                    Position pos;
                    pos.x = ntohs_custom(x);
                    pos.y = ntohs_custom(y);
                    snake.body.push_back(pos);
                }

                // Add player to state
                new_state.players[pid] = snake;
            }

            // Update game state
            std::lock_guard<std::mutex> lock(game_state_mutex);
            game_state = new_state;

            return true;
        } catch (...) {
            return false;
        }
    }

    void listen_for_updates() {
        while (running) {
            // Try to receive data
            socklen_t server_len = sizeof(server_addr);
            int nbytes = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr,
                                  &server_len);

            if (nbytes > 0) {
                // Check if it's a state update
                if (buffer[0] == MSG_STATE_UPDATE) {
                    parse_binary_game_state(buffer, nbytes);
                }
            }

            // Sleep a bit to prevent CPU hogging
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void heartbeat_loop() {
        while (running) {
            send_heartbeat();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void start() {
        // Join the game
        if (!join_game()) {
            std::cerr << "Failed to join game" << std::endl;
            return;
        }

        std::cout << "Joined game as player " << player_id << std::endl;

        running = true;

        // Start listener thread
        std::thread listener_thread(&SnakeClient::listen_for_updates, this);

        // Start heartbeat thread
        std::thread heartbeat_thread(&SnakeClient::heartbeat_loop, this);

        // Start the UI
        run_game_ui();

        // Cleanup
        running = false;
        listener_thread.join();
        heartbeat_thread.join();
    }

    void run_game_ui() {
        // Initialize ncurses
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        curs_set(0);

        // Initialize colors
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);   // My snake
        init_pair(2, COLOR_RED, COLOR_BLACK);     // Food
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Other snakes

        // Game loop
        while (running) {
            // Handle input
            int key = getch();
            if (key == 'q' || key == 'Q') {
                running = false;
                break;
            } else if (key == KEY_UP) {
                send_input("UP");
            } else if (key == KEY_RIGHT) {
                send_input("RIGHT");
            } else if (key == KEY_DOWN) {
                send_input("DOWN");
            } else if (key == KEY_LEFT) {
                send_input("LEFT");
            }

            // Clear screen
            clear();

            // Draw game state
            {
                std::lock_guard<std::mutex> lock(game_state_mutex);
                draw_game_state();
            }

            // Draw instructions
            int max_y, max_x;
            getmaxyx(stdscr, max_y, max_x);
            mvprintw(max_y - 1, 0, "Use arrow keys to move. Press 'q' to quit. Protocol: Binary");

            // Refresh screen
            refresh();

            // Sleep to limit frame rate
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Clean up ncurses
        endwin();
    }

    void draw_game_state() {
        if (grid_size.first == 0 || grid_size.second == 0) {
            return;
        }

        int width = grid_size.first;
        int height = grid_size.second;

        // Draw border
        mvprintw(0, 0, "+");
        for (int x = 0; x < width; x++) {
            mvprintw(0, x + 1, "-");
        }
        mvprintw(0, width + 1, "+");

        for (int y = 0; y < height; y++) {
            mvprintw(y + 1, 0, "|");
            mvprintw(y + 1, width + 1, "|");
        }

        mvprintw(height + 1, 0, "+");
        for (int x = 0; x < width; x++) {
            mvprintw(height + 1, x + 1, "-");
        }
        mvprintw(height + 1, width + 1, "+");

        // Draw food
        attron(COLOR_PAIR(2));
        for (const auto& food : game_state.food) {
            mvprintw(food.y + 1, food.x + 1, "o");
        }
        attroff(COLOR_PAIR(2));

        // Draw players
        for (const auto& player_pair : game_state.players) {
            const std::string& pid = player_pair.first;
            const Snake& snake = player_pair.second;

            // Determine color based on whether it's our snake or not
            int color_pair = (pid == player_id) ? 1 : 3;
            attron(COLOR_PAIR(color_pair));

            // Draw head
            if (!snake.body.empty()) {
                const Position& head = snake.body[0];
                mvprintw(head.y + 1, head.x + 1, "O");

                // Draw body
                for (size_t i = 1; i < snake.body.size(); i++) {
                    const Position& pos = snake.body[i];
                    mvprintw(pos.y + 1, pos.x + 1, "x");
                }
            }

            attroff(COLOR_PAIR(color_pair));
        }

        // Draw score and stats
        auto it = game_state.players.find(player_id);
        if (it != game_state.players.end()) {
            const Snake& my_snake = it->second;
            int score = my_snake.body.size() - 3;  // Starting length is 3
            mvprintw(height + 2, 0, "Score: %d | Players: %zu | Tick: %u", score,
                     game_state.players.size(), game_state.timestamp);
        } else {
            mvprintw(height + 2, 0, "You are dead! Restart to play again.");
        }
    }
};

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string server_host = "127.0.0.1";
    int server_port = 25566;

    if (argc > 1) {
        server_host = argv[1];
    }
    if (argc > 2) {
        server_port = std::stoi(argv[2]);
    }

    // Create and start client
    SnakeClient client(server_host, server_port);
    client.start();

    return 0;
}