#include  "../include/Pack.hpp"
#include <fstream>

using namespace K;

bool Pack::compare(const std::string &a, const std::string &b) {
    const auto h_input = hash(a);
    const std::string hex_input = to_hex(h_input.data(), h_input.size());
    const auto h_output = hash(b);
    const std::string hex_output = to_hex(h_output.data(), h_output.size());
    return h_input == h_output;
}

std::string Pack::to_hex(const uint8_t *data, const size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
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
        const std::streamsize s = ifs.gcount();
        if (s > 0) {
            blake3_hasher_update(&hasher, buf.data(), static_cast<size_t>(s));
        }
    }
    blake3_hasher_finalize(&hasher, out.data(), out.size());
    return out;
}

void Pack::ok(const std::string &message) {
    std::cout << "\033[1;32m* \033[1;37m" << message << "\033[0m" << std::endl;
}

void Pack::ko(const std::string &message) {
    std::cerr << "\033[1;31m! \033[1;37m" << message << "\033[0m" << std::endl;
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

bool Pack::exists(const std::string &file) {
    const std::ifstream f(file, std::ios::binary);
    return f.good();
}
