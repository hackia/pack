#include "../../libs/include/Pack.hpp"
#include "../../libs/include/KeyManager.hpp"
#include <chrono>   // NOUVEAU
#include <cstring>
#include <sstream>  // NOUVEAU
#include <iomanip>  // NOUVEAU
#include <ctime>    // NOUVEAU
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <cctype>

using namespace K;
using namespace std;

const string VERSION = "1.0.0";
constexpr uint16_t DEFAULT_SYNC_PORT = 8080;

namespace {
    int sync(const vector<string> &args) {
        if (args.size() < 3) {
            Pack::ko("sync: require <directory> <host>");
            return Pack::USAGE_ERROR;
        }
        const std::string &directory = args[1];
        const std::string &host = args[2];

        // Verify if the directory exists
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            Pack::ko("sync: '" + directory + "' is not a valid directory");
            return Pack::INPUT_NOT_FOUND;
        }

        const std::string home_dir = getenv("HOME");
        const std::string public_key_path = home_dir + "/.pack/id_ed25519.pub";
        const std::string private_key_path = home_dir + "/.pack/id_ed25519";

        KeyManager km;
        if (!km.loadKeys(public_key_path, private_key_path)) {
            Pack::ko("Could not load keys from ~/.pack/. Please run 'pack keygen' first.");
            return Pack::SYS_ERROR;
        }

        int result = Pack::OK;

        // Iterate through directory
        for (const auto &entry: std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string relative_path = std::filesystem::relative(entry.path(), directory).string();
                Pack::ok("Uploading " + relative_path);

                result = Pack::send_file(entry.path().string(), relative_path, host, DEFAULT_SYNC_PORT,
                                         km.getPublicKey(), km.getPrivateKey(),
                                         Pack::DEFAULT_TIMEOUT);
                if (result != Pack::OK) {
                    Pack::ko("Failed to upload " + relative_path);
                    return result;
                }
            }
        }
        return result;
    }

    void save_history(const string &file, const string &line) {
        ofstream f(file, ios::app);
        f << line << endl;
        f.close();
    }

    vector<string> directories() {
        vector<string> dirs;
        for (const auto &entry: std::filesystem::directory_iterator(".")) {
            if (entry.is_directory() && entry.path().filename().string().starts_with(".") == false) {
                dirs.push_back(entry.path().filename().string());
            }
        }
        return dirs;
    }

    // Read a line from stdin with basic editing and history navigation (Up/Down) using ~/.pack_history
    std::string read_line_with_history(const std::string &prompt, const std::string &history_file) {
        // Load history
        std::vector<std::string> lines;
        if (std::ifstream f(history_file); f.is_open()) {
            std::string l;
            while (std::getline(f, l)) lines.push_back(l);
        }
        size_t index = lines.size(); // one past the last (current input)
        std::string buf;

        // Setup raw terminal
        termios oldt{};
        termios raw{};
        tcgetattr(STDIN_FILENO, &oldt);
        raw = oldt;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        auto refresh = [&] {
            std::cout << "\r" << prompt << buf << "\033[K" << std::flush;
        };

        // Initial render (prompt already printed by caller, but ensure alignment)
        refresh();

        while (true) {
            char c = 0;
            ssize_t r = ::read(STDIN_FILENO, &c, 1);
            if (r <= 0) continue;
            if (c == '\n' || c == '\r') {
                std::cout << "\n";
                break;
            }
            if (static_cast<unsigned char>(c) == 127 || c == '\b') {
                // backspace
                if (!buf.empty()) {
                    buf.pop_back();
                    refresh();
                }
            } else if (c == '\x1b') {
                // escape sequence
                char seq[2] = {0, 0};
                if (::read(STDIN_FILENO, &seq[0], 1) != 1) continue;
                if (::read(STDIN_FILENO, &seq[1], 1) != 1) continue;
                if (seq[0] == '[') {
                    if (seq[1] == 'A') {
                        // Up
                        if (index > 0) {
                            index--;
                            buf = lines[index];
                            refresh();
                        }
                    } else if (seq[1] == 'B') {
                        // Down
                        if (index < lines.size()) {
                            index++;
                            if (index == lines.size()) buf.clear();
                            else buf = lines[index];
                            refresh();
                        }
                    }
                }
            } else if (isprint(static_cast<unsigned char>(c))) {
                buf.push_back(c);
                refresh();
            }
        }

        // Restore terminal
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return buf;
    }
}

void print_help(const char *program) {
    std::cerr << "Usage:\n"
            << "  " << program << " encode <input> <output.hex>         # Encode binary file to hex\n"
            << "  " << program << " decode <input.hex> <output>         # Decode hex file to binary\n"
            << "  " << program << " verify <input> <output.hex>         # Verify encode/decode integrity\n"
            << "  " << program << " send <file> <destination>           # Send file to destination\n"
            << "  " << program <<
            " send-pubkey <destination>           # Send your public key file securely to destination\n"
            << "  " << program << " recv <port>                         # Receive file on port (default: 8080)\n"
            << "  " << program <<
            " keygen                              # Generate a new key pair (id_ed25519, id_ed25519.pub)\n"
            << "  " << program << " list                                # List available files\n"
            << "  " << program << " delete <file> <host> <port>         # Delete file from host\n"
            << "  " << program <<
            " sync <directory> <host>             # Upload file into the directory to the server\n"
            << "  " << program << " help                                # Show this help\n"
            << "  " << program << " version                             # Show version\n";
}

int main(const int argc, const char **argv) {
    if (argc == 1) {
        string input;
        vector<string> args;
        string port;
        string host;
        vector<string> folders;
        const string history = getenv("HOME") + string("/.pack_history");
        // Enter alternate screen
        cout << "\033[?1049h\033[H";

        // Set up signal handlers
        signal(SIGINT, [](int) {
            cout << "\033[?1049l"; // Exit alternate screen
            exit(0);
        });

        while (true) {
            auto now = std::chrono::system_clock::now();
            auto current_time = std::chrono::system_clock::to_time_t(now);
            auto current_path = std::filesystem::current_path();
            const string PROMPT_TIME_COLOR = "\033[1;35m";
            const string PROMPT_BRACKET_COLOR = "\033[1;37m";
            const string PROMPT_NAME_COLOR = "\033[1;32m";
            const string PROMPT_PATH_COLOR = "\033[1;34m";
            const string PROMPT_RESET = "\033[0m";
            folders = directories();
            std::ostringstream prompt_oss;
            if (current_path.filename() == getenv("USER")) {
                prompt_oss << PROMPT_BRACKET_COLOR << "[ "
                        << PROMPT_TIME_COLOR << std::put_time(std::localtime(&current_time), "%H:%M:%S")
                        << PROMPT_BRACKET_COLOR << " ]"
                        << PROMPT_NAME_COLOR << " pack"
                        << PROMPT_BRACKET_COLOR << " ~ "
                        << PROMPT_BRACKET_COLOR << "> "
                        << PROMPT_RESET;
            } else {
                prompt_oss << PROMPT_BRACKET_COLOR << "[ "
                        << PROMPT_TIME_COLOR << std::put_time(std::localtime(&current_time), "%H:%M:%S")
                        << PROMPT_BRACKET_COLOR << " ]"
                        << PROMPT_NAME_COLOR << " pack"
                        << PROMPT_BRACKET_COLOR << " ~ "
                        << PROMPT_PATH_COLOR << current_path.filename().string()
                        << PROMPT_BRACKET_COLOR << "> "
                        << PROMPT_RESET;
            }
            const std::string prompt_str = prompt_oss.str();
            // Print prompt once; the line editor will refresh as needed
            cout << prompt_str << flush;
            input = read_line_with_history(prompt_str, history);
            if (input.empty()) continue;

            istringstream iss(input);
            args.clear();
            string arg;
            while (iss >> arg) {
                args.emplace_back(arg);
            }
            if (args.empty()) {
                continue;
            }
            if (args[0] == "exit" || args[0] == "quit") {
                cout << "\033[?1049l"; // Exit alternate screen
                save_history(history, input);
                break;
            }
            if (args[0] == "help") {
                cout << "version                          # Show version\n";
                cout << "help                             # Show this help\n";
                cout << "cd <directory>                   # checkout of directory\n";
                cout << "ls                               # list of directories\n";
                cout << "clear                            # clear screen\n";
                cout << "cls                              # clear screen\n";
                cout << "history                          # Show history\n";
                cout << "history clear                    # Clear command history\n";
                cout << "history <number>                 # Show history\n";
                cout << "recv <port>                      # Receive file on port (default: 8080)\n";
                cout << "send-pubkey <destination>        # Send your public key file securely to destination\n";
                cout << "list                             # List available files\n";
                cout << "delete <file> <host> <port>      # Delete file from host\n";
                cout << "set port <port>                  # Set port for server\n";
                cout << "set host <host>                  # Set host for server\n";
                cout << "set key <key>                    # Set key for server\n";
                cout << "keygen                           # Generate a new key pair (id_ed25519, id_ed25519.pub)\n";
                cout << "encode <input> <output.hex>      # Encode binary file to hex\n";
                cout << "decode <input.hex> <output>      # Decode hex file to binary\n";
                cout << "verify <input> <output.hex>      # Verify encode/decode integrity\n";
                cout << "send <file> <destination>        # Send file or directory to destination\n";
                cout << "sync <directory> <host>          # Upload files from directory to server\n";
                cout << "sync <directory>                 # Upload files from directory to server\n";
                cout << "exit                             # Exit the application\n";
            }
            if (args[0] == "set") {
                if (args.size() < 3) {
                    Pack::ko("Usage: set port <port> | set host <host>");
                    save_history(history, input);
                } else if (args[1] == "port") {
                    port.assign(args[2]);
                    Pack::ok("Port set to : " + port);
                    save_history(history, input);
                } else if (args[1] == "host") {
                    host.assign(args[2]);
                    Pack::ok("Host set to : " + host);
                    save_history(history, input);
                } else {
                    Pack::ko("Unknown setting: " + args[1]);
                    save_history(history, input);
                }
            }
            if (args[0] == "clear" || args[0] == "cls") {
                cout << "\033[2J\033[1;1H";
                save_history(history, input);
            }
            if (args[0] == "history") {
                if (args.size() >= 2 && args[1] == "clear") {
                    ofstream f(history, ios::trunc);
                    f.close();
                    Pack::ok("History cleared");
                    save_history(history, input);
                } else {
                    if (ifstream f(history); f.is_open()) {
                        string line;
                        vector<string> lines;
                        while (getline(f, line)) {
                            lines.push_back(line);
                        }
                        for (int i = static_cast<int>(lines.size()) - 1, n = 1; i >= 0; --i, ++n) {
                            cout << n << ": " << lines[i] << endl;
                        }
                    }
                    save_history(history, input);
                }
            }
            if (args[0] == "ls") {
                int i = 0;
                cout << "\n";
                Pack::ok("Available directories\n");
                for (const auto &dir: folders) {
                    ++i;
                    if (i % 3 == 0) {
                        cout << "\033[1;35m" << dir << "\033[0m" << endl << endl;
                    } else {
                        cout << "\033[1;35m" << dir << "\033[0m\t";
                    }
                }
                save_history(history, input);
                cout << "\033[0m" << endl;
            }
            if (args[0] == "cd") {
                if (args.size() == 2) {
                    if (std::filesystem::exists(args[1])) {
                        if (std::filesystem::is_directory(args[1])) {
                            Pack::ok("Changing directory to " + args[1]);
                            std::filesystem::current_path(args[1]);
                        } else {
                            Pack::ko("Not a directory");
                        }
                    }
                }
                save_history(history, input);
            }
            if (args[0] == "sync") {
                if (args.size() == 3) {
                    sync(args);
                } else if (args.size() == 2) {
                    if (host.empty()) {
                        Pack::ko("sync: host not set. Use 'set host <host>' first.");
                    } else {
                        vector<string> sync_args = args;
                        sync_args.push_back(host);
                        sync(sync_args);
                    }
                } else {
                    Pack::ko("sync: require <directory> [host]");
                }
                save_history(history, input);
            }
            if (args[0] == "keygen") {
                try {
                    Pack::ok("Generating new Ed25519 key pair...");
                    if (KeyManager manager; manager.generateKeys()) {
                        const char *home_dir = getenv("HOME");
                        if (home_dir == nullptr) {
                            Pack::ko("Could not find HOME environment variable.");
                            return Pack::SYS_ERROR;
                        }
                        std::string pack_dir = std::string(home_dir) + "/.pack";

                        std::filesystem::create_directory(pack_dir);

                        const std::string public_key_path = pack_dir + "/id_ed25519.pub";
                        const std::string private_key_path = pack_dir + "/id_ed25519";
                        if (manager.saveKeys(public_key_path, private_key_path)) {
                            Pack::ok("Success! Keys saved to 'id_ed25519.pub' (public) and 'id_ed25519' (private).");
                            Pack::ok("IMPORTANT: Share the public key, but KEEP THE PRIVATE KEY SECRET!");
                        } else {
                            Pack::ko("Failed to save keys to files.");
                            return Pack::SYS_ERROR;
                        }
                    } else {
                        Pack::ko("Failed to generate keys.");
                        return Pack::SYS_ERROR;
                    }
                } catch (const std::exception &e) {
                    Pack::ko(e.what());
                    return Pack::SYS_ERROR;
                }
                save_history(history, input);
            }
            vector<const char *> c_args;
            for (const auto &a: args) {
                c_args.push_back(a.c_str());
            }
        }
        return Pack::OK;
    }
    if (argc < 2) {
        print_help(argv[0]);
        return Pack::USAGE_ERROR;
    }

    const std::string cmd = argv[1];
    try {
        if (cmd == "help") {
            print_help(argv[0]);
            return Pack::OK;
        }

        if (cmd == "version") {
            Pack::ok("pack version " + VERSION);
            return Pack::OK;
        }

        if (cmd == "keygen") {
            try {
                Pack::ok("Generating new Ed25519 key pair...");
                if (KeyManager manager; manager.generateKeys()) {
                    const char *home_dir = getenv("HOME");
                    if (home_dir == nullptr) {
                        Pack::ko("Could not find HOME environment variable.");
                        return Pack::SYS_ERROR;
                    }
                    std::string pack_dir = std::string(home_dir) + "/.pack";

                    std::filesystem::create_directory(pack_dir);

                    const std::string public_key_path = pack_dir + "/id_ed25519.pub";
                    if (const std::string private_key_path = pack_dir + "/id_ed25519"; manager.saveKeys(
                        public_key_path, private_key_path)) {
                        Pack::ok("Success! Keys saved to 'id_ed25519.pub' (public) and 'id_ed25519' (private).");
                        Pack::ok("IMPORTANT: Share the public key, but KEEP THE PRIVATE KEY SECRET!");
                    } else {
                        Pack::ko("Failed to save keys to files.");
                        return Pack::SYS_ERROR;
                    }
                } else {
                    Pack::ko("Failed to generate keys.");
                    return Pack::SYS_ERROR;
                }
                return Pack::OK;
            } catch (const std::exception &e) {
                Pack::ko(e.what());
                return Pack::SYS_ERROR;
            }
        }
        if (cmd == "encode") {
            if (argc != 4) {
                Pack::ko("encode: require <input> <output.hex>");
                return Pack::INPUT_NOT_FOUND;
            }
            Pack::encode_hex_file(argv[2], argv[3]);
            Pack::ok("encoded to hex " + string(argv[3]));
            return Pack::OK;
        }

        if (cmd == "decode") {
            if (argc != 4) {
                Pack::ko("decode: require <input.hex> <output>");
                return Pack::INPUT_NOT_FOUND;
            }
            Pack::decode_hex_file(argv[2], argv[3]);
            Pack::ok("decoded to binary " + string(argv[3]));
            return Pack::OK;
        }

        if (cmd == "verify") {
            if (argc != 4) {
                Pack::ko("verify: require <input> <output.hex>");
                return Pack::INPUT_NOT_FOUND;
            }
            const std::string input = argv[2];
            const std::string out_hex = argv[3];
            const std::string decoded = out_hex + ".dec";
            Pack::ok("Verifying...");
            Pack::encode_hex_file(input, out_hex);
            Pack::decode_hex_file(out_hex, decoded);
            const auto h_in = Pack::hash(input);
            const auto h_dec = Pack::hash(decoded);
            const std::string hx_in = Pack::to_hex(h_in.data(), h_in.size());
            const std::string hx_dec = Pack::to_hex(h_dec.data(), h_dec.size());
            if (h_in == h_dec) {
                Pack::ok(hx_in + " == " + hx_dec);
                return Pack::OK;
            }
            Pack::ko("Hash(input) != Hash(decoded)");
            return Pack::MISMATCH;
        }

        if (cmd == "send") {
            if (argc != 4) {
                Pack::ko("send: require <file> <destination:port>");
                return Pack::INPUT_NOT_FOUND;
            }
            std::string home_dir = getenv("HOME");
            std::string public_key_path = home_dir + "/.pack/id_ed25519.pub";
            std::string private_key_path = home_dir + "/.pack/id_ed25519";

            KeyManager km;
            if (!km.loadKeys(public_key_path, private_key_path)) {
                Pack::ko("Could not load keys from ~/.pack/. Please run 'pack keygen' first.");
                return Pack::SYS_ERROR;
            }
            const auto &public_key = km.getPublicKey();
            const auto &private_key = km.getPrivateKey();
            std::string file_path = argv[2];
            std::string destination = argv[3];

            if (filesystem::is_directory(file_path)) {
                size_t colon_pos = destination.find(':');
                if (colon_pos == std::string::npos) {
                    Pack::ko("Invalid destination format. Use <host>:<port>");
                    return Pack::USAGE_ERROR;
                }
                std::string host = destination.substr(0, colon_pos);
                uint16_t port = 0;
                try {
                    port = std::stoi(destination.substr(colon_pos + 1));
                } catch (const std::exception &e) {
                    Pack::ko(e.what());
                    return Pack::USAGE_ERROR;
                }
                return Pack::send_directory(file_path, host, port, public_key, private_key, Pack::DEFAULT_TIMEOUT);
            }
            size_t colon_pos = destination.find(':');
            if (colon_pos == std::string::npos) {
                Pack::ko("Invalid destination format. Use <host>:<port>");
                return Pack::USAGE_ERROR;
            }

            std::string host = destination.substr(0, colon_pos);
            uint16_t port = 0;
            try {
                port = std::stoi(destination.substr(colon_pos + 1));
            } catch (const std::exception &e) {
                Pack::ko(e.what());
                return Pack::USAGE_ERROR;
            }

            Pack::ok("Sending " + file_path + " to " + host + ":" + std::to_string(port));
            return Pack::send_file(file_path, host, port, public_key, private_key, Pack::DEFAULT_TIMEOUT);
        }
        if (cmd == "send-pubkey") {
            if (argc != 3) {
                Pack::ko("send-pubkey: require <destination:port>");
                return Pack::INPUT_NOT_FOUND;
            }
            std::string home_dir = getenv("HOME");
            std::string public_key_file = home_dir + "/.pack/id_ed25519.pub";
            const std::string &public_key_path = public_key_file; // file to send
            std::string private_key_path = home_dir + "/.pack/id_ed25519";

            KeyManager km;
            if (!km.loadKeys(public_key_path, private_key_path)) {
                Pack::ko("Could not load keys from ~/.pack/. Please run 'pack keygen' first.");
                return Pack::SYS_ERROR;
            }
            const auto &public_key = km.getPublicKey();
            const auto &private_key = km.getPrivateKey();

            std::string destination = argv[2];
            size_t colon_pos = destination.find(':');
            if (colon_pos == std::string::npos) {
                Pack::ko("Invalid destination format. Use <host>:<port>");
                return Pack::USAGE_ERROR;
            }
            std::string host = destination.substr(0, colon_pos);
            uint16_t port = 0;
            try {
                port = std::stoi(destination.substr(colon_pos + 1));
            } catch (const std::exception &e) {
                Pack::ko(e.what());
                return Pack::USAGE_ERROR;
            }
            Pack::ok("Sending public key file to " + host + ":" + std::to_string(port));
            return Pack::send_file(public_key_file, host, port, public_key, private_key, Pack::DEFAULT_TIMEOUT);
        }
        if (cmd == "recv") {
            try {
                int port = stoi(argv[2]);
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << "recv_" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S") << ".dat";
                std::string output_filename = ss.str();
                Pack::ok(output_filename);
                Pack::receive_file(port, Pack::DEFAULT_TIMEOUT);
            } catch (const std::invalid_argument &e) {
                Pack::ko(e.what());
                return Pack::USAGE_ERROR;
            }
        }

        if (cmd == "list") {
            Pack::ko("list command not implemented yet");
            return Pack::USAGE_ERROR;
        }

        if (cmd == "delete") {
            if (argc != 5) {
                Pack::ko("delete: require <file> <host> <port>");
                return Pack::INPUT_NOT_FOUND;
            }
            try {
                const int port = std::stoi(argv[4]);
                return Pack::delete_file(argv[2], argv[3], port, Pack::DEFAULT_TIMEOUT);
            } catch (const std::invalid_argument &e) {
                Pack::ko(e.what());
                return Pack::USAGE_ERROR;
            }
        }

        if (cmd == "sync") {
            if (argc != 4) {
                Pack::ko("sync: require <directory> <host>");
                return Pack::USAGE_ERROR;
            }
            const std::string directory = argv[2];
            const std::string host = argv[3];

            // Verify if the directory exists
            if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
                Pack::ko("sync: '" + directory + "' is not a valid directory");
                return Pack::INPUT_NOT_FOUND;
            }

            std::string home_dir = getenv("HOME");
            std::string public_key_path = home_dir + "/.pack/id_ed25519.pub";
            std::string private_key_path = home_dir + "/.pack/id_ed25519";

            KeyManager km;
            if (!km.loadKeys(public_key_path, private_key_path)) {
                Pack::ko("Could not load keys from ~/.pack/. Please run 'pack keygen' first.");
                return Pack::SYS_ERROR;
            }

            Pack::ok("Syncing directory " + directory + " to " + host);
            int result = Pack::OK;

            // Iterate through directory
            for (const auto &entry: std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    Pack::ok("Uploading " + entry.path().string());
                    result = Pack::send_file(entry.path().filename(), entry.path().filename().string(), host,
                                             DEFAULT_SYNC_PORT,
                                             km.getPublicKey(), km.getPrivateKey(),
                                             Pack::DEFAULT_TIMEOUT
                    );
                    if (result != Pack::OK) {
                        Pack::ko("Failed to upload " + entry.path().string());
                        return result;
                    }
                }
            }
            return result;
        }
        Pack::ko("Unknown command: " + cmd);
        print_help(argv[0]);
        return Pack::USAGE_ERROR;
    } catch (std::system_error &e) {
        Pack::ko(e.what());
        return Pack::SYS_ERROR;
    } catch (std::exception &e) {
        Pack::ko(e.what());
        return Pack::USAGE_ERROR;
    }
}
