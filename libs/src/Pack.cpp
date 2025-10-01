#include "../include/Pack.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <arpa/inet.h>
#include <bits/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include <sodium.h>
using namespace K;

void Pack::encode_hex_file(const std::string &in, const std::string &out_hex) {
    std::ifstream ifs(in, std::ios::binary);
    if (!ifs) throw std::system_error(errno, std::generic_category(), "open src failed: " + in);
    std::ofstream ofs(out_hex);
    if (!ofs) throw std::system_error(errno, std::generic_category(), "open dst failed: " + out_hex);

    std::vector<unsigned char> buf(BUF_SIZE);
    std::string hex;
    hex.resize(BUF_SIZE * 2);

    while (ifs) {
        ifs.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
        if (const std::streamsize s = ifs.gcount(); s > 0) {
            for (std::streamsize i = 0; i < s; ++i) {
                unsigned v = buf[static_cast<size_t>(i)];
                hex[2 * i] = HEXMAP[(v >> 4) & 0xF];
                hex[2 * i + 1] = HEXMAP[v & 0xF];
            }
            ofs.write(hex.data(), s * 2);
        }
    }
    ofs.flush();
    if (!ofs) throw std::system_error(errno, std::generic_category(), "write failed: " + out_hex);
}

std::string Pack::to_hex(const uint8_t *data, const size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

bool Pack::exists(const std::string &file) {
    const std::ifstream f(file, std::ios::binary);
    return f.good();
}

void Pack::copy(const std::string &dest, const std::string &source) {
    std::ifstream ifs(source, std::ios::binary);
    if (!ifs) throw std::system_error(errno, std::generic_category(), "open src failed: " + source);
    std::ofstream ofs(dest, std::ios::binary | std::ios::trunc);
    if (!ofs) throw std::system_error(errno, std::generic_category(), "open dst failed: " + dest);
    std::vector<char> buf(BUF_SIZE);
    while (ifs) {
        ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        if (const std::streamsize s = ifs.gcount(); s > 0) ofs.write(buf.data(), s);
    }
    ofs.flush();
    if (!ofs) throw std::system_error(errno, std::generic_category(), "write failed: " + dest);
}

uint8_t Pack::from_hex_nibble(const char c) {
    if ('0' <= c && c <= '9') return static_cast<uint8_t>(c - '0');
    if ('a' <= c && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if ('A' <= c && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    throw std::runtime_error(std::string("Invalid hex nibble: '") + c + "'");
}

void Pack::ko(const std::string &message) {
    std::cerr << "\033[1;31m! \033[1;37m" << message << "\033[0m" << std::endl;
}

void Pack::decode_hex_file(const std::string &in_hex, const std::string &out_bin) {
    std::ifstream ifs(in_hex);
    if (!ifs) throw std::system_error(errno, std::generic_category(), "open src failed: " + in_hex);
    std::ofstream ofs(out_bin, std::ios::binary | std::ios::trunc);
    if (!ofs) throw std::system_error(errno, std::generic_category(), "open dst failed: " + out_bin);

    constexpr size_t CHUNK = BUF_SIZE * 2;
    std::string hex;
    hex.resize(CHUNK);
    std::vector<unsigned char> out(BUF_SIZE);

    while (ifs) {
        ifs.read(hex.data(), static_cast<std::streamsize>(hex.size()));
        if (const std::streamsize s = ifs.gcount(); s > 0) {
            if (s % 2 != 0) {
                throw std::runtime_error("Hex input length is odd (invalid).");
            }
            auto out_bytes = static_cast<size_t>(s / 2);
            for (size_t i = 0; i < out_bytes; ++i) {
                uint8_t hi = from_hex_nibble(hex[2 * i]);
                uint8_t lo = from_hex_nibble(hex[2 * i + 1]);
                out[i] = static_cast<uint8_t>((hi << 4) | lo);
            }
            ofs.write(reinterpret_cast<const char *>(out.data()), static_cast<std::streamsize>(out_bytes));
        }
    }
    ofs.flush();
    if (!ofs) throw std::system_error(errno, std::generic_category(), "write failed: " + out_bin);
}


void Pack::ok(const std::string &message) {
    std::cout << "\033[1;32m* \033[1;37m" << message << "\033[0m" << std::endl;
}

std::vector<uint8_t> Pack::hash(const std::string &file) {
    std::vector<uint8_t> out(DIGEST_SIZE);
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) throw std::system_error(errno, std::generic_category(), "open for read failed: " + file);

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    std::vector<char> buf(BUF_SIZE);
    while (ifs) {
        ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        if (const std::streamsize s = ifs.gcount(); s > 0) {
            blake3_hasher_update(&hasher, buf.data(), static_cast<size_t>(s));
        }
    }
    blake3_hasher_finalize(&hasher, out.data(), out.size());
    return out;
}

int Pack::send_file(const std::string &file_path, const std::string &host, uint16_t port,
                    const std::vector<unsigned char> &publicKey, const std::vector<unsigned char> &privateKey,
                    unsigned int timeout) {
    if (!std::filesystem::exists(file_path)) {
        ko("Input file not found: " + file_path);
        return INPUT_NOT_FOUND;
    }

    auto file_hash = hash(file_path);

    std::vector<unsigned char> signature(crypto_sign_ed25519_BYTES);
    crypto_sign_detached(signature.data(), nullptr,
                         file_hash.data(), file_hash.size(),
                         privateKey.data());
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ko("Socket creation error");
        return NETWORK_ERROR;
    }

    timeval tv{};
    tv.tv_sec = timeout;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        ko("Invalid address or address not supported");
        close(sock);
        return NETWORK_ERROR;
    }

    if (connect(sock, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        ko("Connection Failed");
        close(sock);
        return NETWORK_ERROR;
    }

    std::string base_filename = std::filesystem::path(file_path).filename().string();
    if (send(sock, base_filename.c_str(), base_filename.length() + 1, 0) < 0) {
        ko("Failed to send filename");
        close(sock);
        return NETWORK_ERROR;
    }
    send(sock, publicKey.data(), publicKey.size(), 0);
    send(sock, signature.data(), signature.size(), 0);
    send(sock, base_filename.c_str(), base_filename.length() + 1, 0);
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) {
        ko("Failed to open file for reading: " + file_path);
        close(sock);
        return SYS_ERROR;
    }

    std::vector<char> buffer(NET_BUF_SIZE);
    while (ifs.read(buffer.data(), static_cast<streamsize>(buffer.size()))) {
        if (send(sock, buffer.data(), static_cast<size_t>(ifs.gcount()), 0) < 0) {
            ko("Failed to send file content");
            close(sock);
            return NETWORK_ERROR;
        }
    }
    if (ifs.gcount() > 0) {
        if (send(sock, buffer.data(), static_cast<size_t>(ifs.gcount()), 0) < 0) {
            ko("Failed to send final file content chunk");
            close(sock);
            return NETWORK_ERROR;
        }
    }
    ifs.close();
    ok("File sent successfully.");
    close(sock);
    return OK;
}

int Pack::send_directory(const std::string &file_path, const std::string &host, uint16_t port,
                         const std::vector<unsigned char> &publicKey, const std::vector<unsigned char> &privateKey,
                         unsigned int timeout) {
    if (!std::filesystem::exists(file_path)) {
        ko("Directory not found: " + file_path);
        return INPUT_NOT_FOUND;
    }

    if (!std::filesystem::is_directory(file_path)) {
        ko("Not a directory: " + file_path);
        return INPUT_NOT_FOUND;
    }

    std::vector<std::string> ignore_patterns;
    if (const std::string ignore_file = file_path + "/.packignore"; std::filesystem::exists(ignore_file)) {
        std::ifstream ifs(ignore_file);
        std::string line;
        while (std::getline(ifs, line)) {
            if (!line.empty() && line[0] != '#') {
                ignore_patterns.push_back(line);
            }
        }
    }

    auto should_ignore = [&ignore_patterns](const std::string &path) {
        for (const auto &pattern: ignore_patterns) {
            if (path.find(pattern) != std::string::npos) {
                return true;
            }
        }
        return false;
    };

    int result = OK;
    for (const auto &entry: std::filesystem::recursive_directory_iterator(file_path)) {
        if (entry.is_regular_file()) {
            if (std::string relative_path = std::filesystem::relative(entry.path(), file_path).string(); !should_ignore(
                relative_path)) {
                ok("Sending file: " + relative_path);
                if (const int send_result = send_file(entry.path().string(), host, port, publicKey, privateKey, timeout)
                    ; send_result != OK) {
                    ko("Failed to send: " + relative_path);
                    result = send_result;
                }
            }
        }
    }
    return result;
}

int Pack::receive_file(uint16_t port, unsigned int timeout) {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        ko("Socket creation error");
        return NETWORK_ERROR;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        ko("Bind failed");
        close(server_fd);
        return NETWORK_ERROR;
    }

    if (listen(server_fd, 10) < 0) {
        ko("Listen failed");
        close(server_fd);
        return NETWORK_ERROR;
    }

    ok("Server listening continuously on port " + std::to_string(port));

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_sock = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

        if (client_sock < 0) {
            ko("Accept failed, continuing to listen...");
            continue;
        }
        ok("Accepted connection from " + std::string(inet_ntoa(client_addr.sin_addr)));
        std::vector<unsigned char> sender_public_key(crypto_sign_ed25519_PUBLICKEYBYTES);
        recv(client_sock, sender_public_key.data(), sender_public_key.size(), MSG_WAITALL);

        std::vector<unsigned char> signature(crypto_sign_ed25519_BYTES);
        recv(client_sock, signature.data(), signature.size(), MSG_WAITALL);
        std::vector<unsigned char> file_hash(DIGEST_SIZE);
        char c;
        timeval tv{};
        tv.tv_sec = timeout;
        setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::string original_filename;
        while (recv(client_sock, &c, 1, 0) > 0 && c != '\0') {
            original_filename += c;
        }

        if (original_filename.empty()) {
            ko("Client from " + std::string(client_ip) + " did not send a filename. Closing connection.");
            close(client_sock);
            continue;
        }

        std::filesystem::path p(original_filename);
        std::string stem = p.stem().string();
        std::string ext = p.extension().string();

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss_date;
        ss_date << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
        std::string final_filename = stem + "_" + ss_date.str() + string(ext);

        std::ofstream ofs(final_filename, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            ko("Could not open file for writing: " + final_filename);
            close(client_sock);
            continue;
        }


        std::vector<uint8_t> buffer(NET_BUF_SIZE);
        size_t total_received = 0;
        ssize_t bytes_received;

        while ((bytes_received = recv(client_sock, buffer.data(), buffer.size(), 0)) > 0) {
            ofs.write(reinterpret_cast<char *>(buffer.data()), bytes_received);
            total_received += bytes_received;
        }
        ofs.close();
        close(client_sock);
        ok("Finished with client " + std::string(client_ip) + ". Received " + std::to_string(total_received) +
           " bytes into " + final_filename);
        ok("Verifying signature...");
        auto received_hash = hash(final_filename);
        if (crypto_sign_verify_detached(signature.data(),
                                        received_hash.data(), received_hash.size(),
                                        sender_public_key.data()) == 0) {
            ok("Signature is valid. Transfer complete.");
        } else {
            ko("!!! SIGNATURE VERIFICATION FAILED !!! The file is corrupted or not authentic.");
            std::filesystem::remove(final_filename);
        }
        ok("Waiting for new connection...");
    }

    close(server_fd);
    return OK;
}

bool Pack::verify_transfer(const std::vector<uint8_t> &original_hash, const std::string &received_file) {
    if (!filesystem::exists(received_file)) return false;
    const auto received_hash = hash(received_file);
    return original_hash == received_hash;
}

int Pack::delete_file(const char *file_path, const char *host, const uint16_t port, const unsigned int timeout) {
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NETWORK_ERROR;

    timeval tv{};
    tv.tv_sec = timeout;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        close(sock);
        return NETWORK_ERROR;
    }

    if (connect(sock, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        close(sock);
        return NETWORK_ERROR;
    }

    const std::string delete_cmd = "DELETE " + string(file_path) + "\r\n";
    if (send(sock, delete_cmd.c_str(), delete_cmd.length(), 0) < 0) {
        close(sock);
        return NETWORK_ERROR;
    }

    char response[128];
    const ssize_t bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    if (bytes_received <= 0) return NETWORK_ERROR;
    response[bytes_received] = '\0';

    return strncmp(response, "OK", 2) == 0 ? OK : SYS_ERROR;
}


std::vector<uint8_t> Pack::prepare_file_chunk(const std::string &file_path, size_t offset, size_t chunk_size) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) throw std::system_error(errno, std::generic_category(), "Failed to open file: " + file_path);

    ifs.seekg(static_cast<streamoff>(offset));
    std::vector<uint8_t> chunk(chunk_size);
    ifs.read(reinterpret_cast<char *>(chunk.data()), static_cast<streamsize>(chunk_size));
    chunk.resize(ifs.gcount());
    return chunk;
}

bool Pack::write_chunk(const std::string &file_path, const std::vector<uint8_t> &chunk, size_t offset) {
    std::ofstream ofs(file_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!ofs) return false;

    ofs.seekp(static_cast<streamoff>(offset));
    ofs.write(reinterpret_cast<const char *>(chunk.data()), static_cast<streamsize>(chunk.size()));
    return ofs.good();
}
