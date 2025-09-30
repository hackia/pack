#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>


extern "C" {
#include <blake3.h>
}

using namespace std;

namespace K {
    static constexpr char HEXMAP[] = "0123456789abcdef";

    class Pack {
    public:
        static constexpr size_t BUF_SIZE = 64 * 1024; // 64 KiB
        static constexpr size_t DIGEST_SIZE = 32; // BLAKE3 default 32 bytes
        static constexpr int HASH_EQUALS = 0;
        static constexpr int HASH_UNEQUALS = 1;
        static constexpr int INPUT_NOT_FOUND = 2;
        static constexpr int SYS_ERROR = 3;
        static constexpr int MISMATCH = 4;
        static constexpr int USAGE_ERROR = 5;
        static constexpr int OK = 0;

        static bool compare(const std::string &a, const std::string &b);

        static uint8_t from_hex_nibble(char c);

        static void encode_hex_file(const std::string &in, const std::string &out_hex);

        static std::string to_hex(const uint8_t *data, size_t len);

        static bool exists(const std::string &file);

        static void copy(const std::string &dest, const std::string &source);

        static void ko(const std::string &message);

        static void decode_hex_file(const std::string &in_hex, const std::string &out_bin);

        static void ok(const std::string &message);

        static std::vector<uint8_t> hash(const std::string &file);

    private:
        static string input;
        static string output;
    };
}
