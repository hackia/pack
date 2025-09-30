#include "../../libs/include/Pack.hpp"
#include <chrono>   // NOUVEAU
#include <sstream>  // NOUVEAU
#include <iomanip>  // NOUVEAU
#include <ctime>    // NOUVEAU
using namespace K;
using namespace std;

const string VERSION = "1.0.0";

void print_help(const char *program) {
    std::cerr << "Usage:\n"
            << "  " << program << " encode <input> <output.hex>         # Encode binary file to hex\n"
            << "  " << program << " decode <input.hex> <output>         # Decode hex file to binary\n"
            << "  " << program << " verify <input> <output.hex>         # Verify encode/decode integrity\n"
            << "  " << program << " send <file> <destination>           # Send file to destination\n"
            << "  " << program << " recv <file> [port]                  # Receive file on port (default: 8080)\n"
            << "  " << program << " list                                # List available files\n"
            << "  " << program << " delete <file> <host> <port>         # Delete file from host\n"
            << "  " << program << " help                                # Show this help\n"
            << "  " << program << " version                             # Show version\n";
}

int main(const int argc, const char **argv) {
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
            Pack::ko("Hash(input)  != Hash(decoded)");
            return Pack::MISMATCH;
        }

        // Le code CORRIGÉ à mettre dans main.cpp
        if (cmd == "send") {
            if (argc != 4) {
                Pack::ko("send: require <file> <destination:port>");
                return Pack::INPUT_NOT_FOUND;
            }

            std::string file_path = argv[2];
            std::string destination = argv[3];

            size_t colon_pos = destination.find(':');
            if (colon_pos == std::string::npos) {
                Pack::ko("Invalid destination format. Use <host>:<port>");
                return Pack::USAGE_ERROR;
            }

            std::string host = destination.substr(0, colon_pos);
            uint16_t port = 0;
            try {
                port = std::stoi(destination.substr(colon_pos + 1));
            } catch (const std::exception& e) {
                Pack::ko(e.what());
                return Pack::USAGE_ERROR;
            }

            Pack::ok("Sending " + file_path + " to " + host + ":" + std::to_string(port));
            return Pack::send_file(file_path, host, port, Pack::DEFAULT_TIMEOUT);
        }
        if (cmd == "recv") {
            int port = 8080;

            if (argc > 3) {
                try {
                    port = std::stoi(argv[3]);
                } catch (const std::invalid_argument &e) {
                    Pack::ko(e.what());
                    return Pack::USAGE_ERROR;
                }
            }

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "recv_" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S") << ".dat";
            std::string output_filename = ss.str();
            Pack::ok(output_filename);
            Pack::receive_file(output_filename, port, Pack::DEFAULT_TIMEOUT);
            return Pack::OK;
        }

        if (cmd == "list") {
            Pack::ko("list command not implemented yet");
            return Pack::USAGE_ERROR;
        }

        if (cmd == "delete") {
            if (argc != 4) {
                Pack::ko("delete: require <file> <host> <port>");
                return Pack::INPUT_NOT_FOUND;
            }
            try {
                const int port = std::stoi(argv[3]);
                return Pack::delete_file(argv[1], argv[2], port, Pack::DEFAULT_TIMEOUT);
            } catch (const std::invalid_argument &e) {
                Pack::ko(e.what());
                return Pack::USAGE_ERROR;
            }
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
