#include "../include/Pack.hpp"

#include <filesystem>
#include <unistd.h>
#include <arpa/inet.h>
#include <bits/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>

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

uint8_t Pack::from_hex_nibble(const char c) {
    if ('0' <= c && c <= '9') return static_cast<uint8_t>(c - '0');
    if ('a' <= c && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if ('A' <= c && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    throw std::runtime_error(std::string("Invalid hex nibble: '") + c + "'");
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
void Pack::ok(const std::string &message) {
    std::cout << "\033[1;32m* \033[1;37m" << message << "\033[0m" << std::endl;
}

std::vector<uint8_t> Pack::prepare_file_chunk(const std::string &file_path, size_t offset, size_t chunk_size) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) throw std::system_error(errno, std::generic_category(), "Failed to open file: " + file_path);

    ifs.seekg(offset);
    std::vector<uint8_t> chunk(chunk_size);
    ifs.read(reinterpret_cast<char *>(chunk.data()), chunk_size);
    chunk.resize(ifs.gcount());
    return chunk;
}

bool Pack::write_chunk(const std::string &file_path, const std::vector<uint8_t> &chunk, size_t offset) {
    std::ofstream ofs(file_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!ofs) return false;

    ofs.seekp(offset);
    ofs.write(reinterpret_cast<const char *>(chunk.data()), chunk.size());
    return ofs.good();
}

int Pack::send_file(const std::string &file_path, const std::string &host, uint16_t port, unsigned int timeout) {
    if (!filesystem::exists(file_path)) return INPUT_NOT_FOUND;

    std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
    if (!ifs) return SYS_ERROR;

    const size_t file_size = ifs.tellg();
    ifs.close();

    const auto file_hash = hash(file_path);
    TransferMetadata metadata{
        file_size, file_hash,
        static_cast<uint32_t>((file_size + NET_BUF_SIZE - 1) / NET_BUF_SIZE), 1,
    };

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NETWORK_ERROR;

    struct timeval tv{};
    tv.tv_sec = timeout;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        close(sock);
        return NETWORK_ERROR;
    }

    if (connect(sock, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        close(sock);
        return NETWORK_ERROR;
    }

    for (size_t offset = 0; offset < file_size; offset += NET_BUF_SIZE) {
        auto chunk = prepare_file_chunk(file_path, offset, NET_BUF_SIZE);
        if (send(sock, chunk.data(), chunk.size(), 0) < 0) {
            close(sock);
            return NETWORK_ERROR;
        }
    }

    close(sock);
    return OK;
}

int Pack::receive_file(const std::string &output_path, uint16_t port, unsigned int timeout) {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return NETWORK_ERROR;

    struct timeval tv{};
    tv.tv_sec = timeout;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(server_fd);
        return NETWORK_ERROR;
    }

    if (listen(server_fd, 1) < 0) {
        close(server_fd);
        return NETWORK_ERROR;
    }

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    const int client_sock = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

    if (client_sock < 0) {
        close(server_fd);
        return NETWORK_ERROR;
    }

    std::vector<uint8_t> buffer(NET_BUF_SIZE);
    size_t total_received = 0;

    std::ofstream ofs(output_path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        close(client_sock);
        close(server_fd);
        return SYS_ERROR;
    }

    while (true) {
        const ssize_t bytes_received = recv(client_sock, buffer.data(), buffer.size(), 0);
        if (bytes_received <= 0) break;

        ofs.write(reinterpret_cast<char *>(buffer.data()), bytes_received);
        total_received += bytes_received;
    }

    close(client_sock);
    close(server_fd);
    return ofs.good() ? OK : SYS_ERROR;
}

bool Pack::verify_transfer(const std::vector<uint8_t> &original_hash, const std::string &received_file) {
    if (!filesystem::exists(received_file)) return false;
    const auto received_hash = hash(received_file);
    return original_hash == received_hash;
}
