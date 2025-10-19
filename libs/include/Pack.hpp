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
    /** Hexadecimal character map for encoding */
    static constexpr char HEXMAP[] = "0123456789abcdef";

    /**
     * @brief Class handling file operations, encoding/decoding, and network transfers
     */
    class Pack {
    public:
        // Buffer and status constants
        static constexpr size_t BUF_SIZE = 64 * 1024; ///< Buffer size for file operations (64 KiB)
        static constexpr size_t DIGEST_SIZE = 32; ///< BLAKE3 hash digest size in bytes
        static constexpr int HASH_EQUALS = 0; ///< Hash comparison result: equal
        static constexpr int HASH_UNEQUALS = 1; ///< Hash comparison result: not equal
        static constexpr int INPUT_NOT_FOUND = 2; ///< Error: input file not found
        static constexpr int SYS_ERROR = 3; ///< Error: system error occurred
        static constexpr int MISMATCH = 4; ///< Error: data mismatch
        static constexpr int USAGE_ERROR = 5; ///< Error: incorrect usage
        static constexpr int OK = 0; ///< Operation successful

        // Network-related constants
        static constexpr size_t NET_BUF_SIZE = 16 * 1024; ///< Network buffer size (16 KiB)
        static constexpr int NETWORK_ERROR = 6; ///< Error: network operation failed
        static constexpr int TIMEOUT_ERROR = 7; ///< Error: operation timed out
        static constexpr unsigned int DEFAULT_TIMEOUT = 30; ///< Default network timeout in seconds

        /**
         * @brief Encodes a binary file to hexadecimal format
         * @param in Input binary file path
         * @param out_hex Output hexadecimal file path
         */
        static void encode_hex_file(const std::string &in, const std::string &out_hex);

        /**
         * @brief Converts binary data to hexadecimal string
         * @param data Pointer to binary data
         * @param len Length of data in bytes
         * @return Hexadecimal string representation
         */
        static std::string to_hex(const uint8_t *data, size_t len);

        /**
         * @brief Checks if a file exists
         * @param file Path to file
         * @return true if a file exists, false otherwise
         */
        static bool exists(const std::string &file);

        /**
         * @brief Copies a file from source to destination
         * @param dest Destination path
         * @param source Source path
         */
        static void copy(const std::string &dest, const std::string &source);

        /**
         * @brief Converts a hex character to its 4-bit value
         * @param c Hex character
         * @return 4-bit value
         */
        static uint8_t from_hex_nibble(char c);

        /**
         * @brief Prints error message
         * @param message Error message to display
         */
        static void ko(const std::string &message);

        /**
         * @brief Decodes a hexadecimal file to binary
         * @param in_hex Input hex file path
         * @param out_bin Output binary file path
         */
        static void decode_hex_file(const std::string &in_hex, const std::string &out_bin);

        /**
         * @brief Prints success message
         * @param message Success message to display
         */
        static void ok(const std::string &message);

        /**
         * @brief Computes BLAKE3 hash of a file
         * @param file Path to file
         * @return Vector containing a hash value
         */
        static std::vector<uint8_t> hash(const std::string &file);

        /**
         * Sends a file to a remote host over a specified network connection.
         *
         * @param file_path The path to the file to be sent.
         * @param host The hostname or IP address of the remote server.
         * @param port The port number on which the remote server is listening to.
         * @param publicKey The public key used for the connection, typically for verification.
         * @param privateKey The private key used for signing the file hash.
         * @param timeout The timeout duration (in seconds) for network operations.
         * @return An integer status code indicating the result of the operation.
         *         Possible values are INPUT_NOT_FOUND, NETWORK_ERROR, or OK on success.
         */
        static int send_file(const std::string &file_path, const std::string &host,
                             uint16_t port, const std::vector<unsigned char> &publicKey,
                             const std::vector<unsigned char> &privateKey,
                             unsigned int timeout = DEFAULT_TIMEOUT);

        /**
         * Sends the contents of a specified directory to a remote host over a given port.
         *
         * @param file_path The path to the directory to be sent. Must point to a valid directory.
         * @param host The hostname or IP address of the remote server.
         * @param port The port number of the remote server connection.
         * @param publicKey The public key for encryption during the transmission.
         * @param privateKey The private key for encryption during the transmission.
         * @param timeout The timeout period for the connection in milliseconds.
         * @return The status code of the operation. Returns `OK` on success or an error code on failure.
         */
        static int send_directory(const std::string &file_path, const std::string &host, uint16_t port,
                                  const std::vector<unsigned char> &publicKey,
                                  const std::vector<unsigned char> &privateKey,
                                  unsigned int timeout);

        /**
         * @brief Receives a file over network
         * @param port Port to listen on
         * @param timeout Reception timeout in seconds
         * @return Status code
         */
        [[noreturn]] static void receive_file(uint16_t port, unsigned int timeout = DEFAULT_TIMEOUT);

        /**
         * @brief Verifies file transfer integrity
         * @param original_hash Hash of an original file
         * @param received_file Path to a received file
         * @return true if verification successful, false otherwise
         */
        static bool verify_transfer(const std::vector<uint8_t> &original_hash,
                                    const std::string &received_file);

        /**
         * Deletes a file on a remote server using a network connection.
         *
         * @param file_path The path of the file to be deleted.
         * @param host The hostname or IP address of the remote server.
         * @param port The port number to connect to on the remote server.
         * @param timeout The timeout duration (in seconds) for the operation.
         * @return An integer indicating the result of the operation.
         * Returns OK on success, NETWORK_ERROR for network-related failures,
         * or SYS_ERROR for other system-level issues.
         */
        static int delete_file(const char *file_path, const char *host, uint16_t port, unsigned int timeout);

    protected:
        /**
         * @brief Prepares a chunk of file for network transfer
         * @param file_path Path to a source file
         * @param offset Offset in a file
         * @param chunk_size Size of chunk to prepare
         * @return Vector containing chunk data
         */
        static std::vector<uint8_t> prepare_file_chunk(const std::string &file_path,
                                                       size_t offset, size_t chunk_size);

        /**
         * @brief Writes a chunk of received data to a file
         * @param file_path Path to a destination file
         * @param chunk Chunk data to write
         * @param offset Offset in the file to write at
         * @return true if write successful, false otherwise
         */
        static bool write_chunk(const std::string &file_path, const std::vector<uint8_t> &chunk,
                                size_t offset);

    private:
        static std::string input; ///< Input file path
        static std::string output; ///< Output file path

        /**
         * @brief Structure for network transfer metadata
         */
        struct TransferMetadata {
            size_t file_size; ///< Size of a file being transferred
            std::vector<uint8_t> file_hash; ///< Hash of a file for verification
            uint32_t chunk_count; ///< Number of chunks in transfer
            uint16_t protocol_version; ///< Protocol version number
        };
    };
}
