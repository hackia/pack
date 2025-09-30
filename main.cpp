#include "libs/include/Pack.hpp"

using namespace K;

int main(const int argc, const char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input> <output>\n";
        return 2;
    }
    const std::string input = argv[1];
    const std::string output = argv[2];

    try {
        if (!Pack::exists(input)) {
            Pack::ko("Input file not found: " + input);
            return Pack::INPUT_NOT_FOUND;
        }
        Pack::ok("Input file found: " + input);
        Pack::ok("Output file: " + output);
        Pack::ok("Starting process");
        Pack::ok("Calculating hash of input file");
        const auto h_input = Pack::hash(input);
        const std::string hex_input = Pack::to_hex(h_input.data(), h_input.size());
        Pack::ok(hex_input);
        Pack::ok("Copying input file to output file");
        Pack::copy(output, input);
        if (Pack::exists(output)) {
            Pack::ok("Output file created");
            Pack::ok("Copying done");
            Pack::ok("Calculating hash of output file");
            const auto h_output = Pack::hash(output);
            const std::string hex_output = Pack::to_hex(h_output.data(), h_output.size());
            Pack::ok(hex_output);
            const bool equals = h_input == h_output;
            const std::string equals_str = equals ? "true" : "false";
            Pack::ok("hash equals: " + equals_str);
            Pack::ok("Process done");
            return equals ? Pack::HASH_EQUALS : Pack::HASH_UNEQUALS;
        }
    } catch (const std::system_error &e) {
        Pack::ko(e.what());
        return Pack::SYS_ERROR;
    } catch (const std::exception &e) {
        Pack::ko(e.what());
        return Pack::MISMATCH;
    }
}
