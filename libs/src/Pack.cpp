#include  "../include/Pack.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <system_error>

using namespace K;
using namespace std;

bool Pack::compare(const std::string &a, const std::string &b) {
    const auto h_input = hash(a);
    const auto h_output = hash(b);
    return h_input == h_output;
}

uint8_t Pack::from_hex_nibble(const char c) {
    if ('0' <= c && c <= '9') return static_cast<uint8_t>(c - '0');
    if ('a' <= c && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if ('A' <= c && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    throw std::runtime_error(std::string("Invalid hex nibble: '") + c + "'");
}

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
