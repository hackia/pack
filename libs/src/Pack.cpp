#include  "../include/Pack.hpp"

#include <filesystem>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/ioctl.h>

using namespace k;
using namespace std;
namespace fs = std::filesystem;

void Pack::encode_hex_file(const string &in, const string &out_hex) {
    ifstream ifs(in.c_str(), ios::binary);
    if (!ifs) {
        throw system_error(errno, generic_category(), "open src failed: " + in);
    }
    ofstream ofs(out_hex);
    if (!ofs) {
        throw system_error(errno, generic_category(), "open dst failed: " + out_hex);
    }

    vector<unsigned char> buf(BUF_SIZE);
    string hex;
    hex.resize(BUF_SIZE * 2);

    while (ifs) {
        ifs.read(reinterpret_cast<char *>(buf.data()), static_cast<streamsize>(buf.size()));
        if (const streamsize s = ifs.gcount(); s > 0) {
            for (streamsize i = 0; i < s; ++i) {
                unsigned v = buf[static_cast<size_t>(i)];
                auto index = static_cast<size_t>(static_cast<streamsize>(2)) * static_cast<size_t>(i);
                hex[index] = HEXMAP[v >> 4 & 0xF];
                hex[index + 1] = HEXMAP[v & 0xF];
            }
            ofs.write(hex.data(), s * 2);
        }
    }
    ofs.flush();
    if (!ofs) {
        throw system_error(errno, generic_category(), "write failed to " + out_hex);
    }
}

string Pack::to_hex(const uint8_t *data, const size_t len) {
    ostringstream oss;
    oss << hex << setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

bool Pack::exists(const string &file) {
    const ifstream f(file.c_str(), ios::binary);
    return f.good();
}

void Pack::copy(const string &dest, const string &source) {
    ifstream ifs(source.c_str(), ios::binary);
    if (!ifs) {
        throw system_error(errno, generic_category(), "open src failed: " + source);
    }
    ofstream ofs(dest, ios::binary | ios::trunc);
    if (!ofs) {
        throw system_error(errno, generic_category(), "open dst failed: " + dest);
    }
    vector<char> buf(BUF_SIZE);
    while (ifs) {
        ifs.read(buf.data(), static_cast<streamsize>(buf.size()));
        if (const streamsize s = ifs.gcount(); s > 0) ofs.write(buf.data(), s);
    }
    ofs.flush();
    if (!ofs) {
        throw system_error(errno, generic_category(), "write failed: " + dest);
    }
}

uint8_t Pack::from_hex_nibble(const char c) {
    if ('0' <= c && c <= '9') return static_cast<uint8_t>(c - '0');
    if ('a' <= c && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if ('A' <= c && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    throw runtime_error(string("Invalid hex nibble: '") + c + "'");
}

int Pack::ko(const string &message, const int e) {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    const size_t width = w.ws_col;
    const size_t len = message.length();
    const size_t padding = width - len - 10;
    cout << "\033[1;31m ! \033[1;37m" << message << "\033[0m";
    for (size_t i = 0; i < padding; ++i) {
        cout << " ";
    }
    cout << "\033[1;34m[\033[1;31m ko \033[1;34m]\033[0m" << endl;
    return e;
}

void Pack::decode_hex_file(const string &in_hex, const string &out_bin) {
    ifstream ifs(in_hex.c_str());
    if (!ifs) {
        throw system_error(errno, generic_category(), "open src failed: " + in_hex);
    }
    ofstream ofs(out_bin.c_str(), ios::binary | ios::trunc);
    if (!ofs) {
        throw system_error(errno, generic_category(), "open dst failed: " + out_bin);
    }
    constexpr size_t CHUNK = BUF_SIZE * 2;
    string hex;
    hex.resize(CHUNK);
    vector<unsigned char> out(BUF_SIZE);
    while (ifs) {
        ifs.read(hex.data(), static_cast<streamsize>(hex.size()));
        if (const streamsize s = ifs.gcount(); s > 0) {
            if (s % 2 != 0) {
                throw runtime_error("Hex input length is odd (invalid).");
            }
            auto out_bytes = static_cast<size_t>(s / 2);
            for (size_t i = 0; i < out_bytes; ++i) {
                uint8_t hi = from_hex_nibble(hex[2 * i]);
                uint8_t lo = from_hex_nibble(hex[2 * i + 1]);
                out[i] = static_cast<uint8_t>(hi << 4 | lo);
            }
            ofs.write(reinterpret_cast<const char *>(out.data()), static_cast<streamsize>(out_bytes));
        }
    }
    ofs.flush();
    if (!ofs) {
        throw system_error(errno, generic_category(), "write failed: " + out_bin);
    }
}

void Pack::ok(const string &message) {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    const size_t width = w.ws_col;
    const size_t len = message.length();
    const size_t padding = width - len - 10;
    std::cout << "\033[1;32m * \033[1;37m" << message << "\033[0m";
    for (size_t i = 0; i < padding; ++i) {
        std::cout << " ";
    }
    std::cout << "\033[1;34m[\033[1;32m ok \033[1;34m]\033[0m" << std::endl;
}

vector<uint8_t> Pack::hash(const string &file) {
    std::vector<uint8_t> out(DIGEST_SIZE);
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) {
        throw std::system_error(errno, std::generic_category(), "open for read failed: " + file);
    }
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

bool Pack::verify_transfer(const std::vector<uint8_t> &original_hash, const std::string &received_file) {
    return fs::exists(received_file.c_str()) && original_hash == hash(received_file);
}

bool Pack::write_chunk(const std::string &file_path, const std::vector<uint8_t> &chunk, const size_t offset) {
    std::ofstream ofs(file_path.c_str(), std::ios::binary | std::ios::in | std::ios::out);
    if (!ofs) {
        return false;
    }
    ofs.seekp(static_cast<streamoff>(offset));
    ofs.write(reinterpret_cast<const char *>(chunk.data()), static_cast<streamsize>(chunk.size()));
    return ofs.good();
}
