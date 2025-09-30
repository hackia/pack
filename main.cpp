#include "libs/include/Pack.hpp"

extern "C" {
#include <blake3.h>
}

using namespace K;
using namespace std;

int main(const int argc, const char **argv) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                << "  " << argv[0] << " encode <input> <output.hex>\n"
                << "  " << argv[0] << " decode <input.hex> <output>\n"
                << "  " << argv[0] << " verify <input> <output.hex>\n";
        return Pack::USAGE_ERROR;
    }

    const std::string cmd = argv[1];
    try {
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
                Pack::ok("hashes match");
                return Pack::OK;
            }
            Pack::ko("Hashes do not match");
            return Pack::MISMATCH;
        }
    } catch (std::system_error &e) {
        Pack::ko(e.what());
        return Pack::SYS_ERROR;
    } catch (std::exception &e) {
        Pack::ko(e.what());
        return Pack::USAGE_ERROR;
    }
}
