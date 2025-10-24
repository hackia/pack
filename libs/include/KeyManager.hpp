#pragma once

#include <string>
#include <vector>

using namespace std;
namespace k {
    class KeyManager {
    public:
        /**
         * @brief Constructor that initializes libsodium cryptographic library
         * @throws runtime_error if libsodium initialization fails
         */
        KeyManager();

        /**
         * @brief Generates a new Ed25519 public/private key pair
         * @return true if key generation succeeds, false otherwise
         */
        bool generateKeys();

        /**
         * @brief Saves the current key pair to files
         * @param publicKeyPath Path where the public key will be saved
         * @param privateKeyPath Path where the private key will be saved
         * @return true if both keys are successfully saved, false if any operation fails
         */
        [[nodiscard]] bool saveKeys(const string &publicKeyPath, const string &privateKeyPath) const;

        /**
         * @brief Loads a key pair from files
         * @param publicKeyPath Path to the public key file
         * @param privateKeyPath Path to the private key file
         * @return true if both keys are successfully loaded and have valid sizes, false otherwise
         */
        bool loadKeys(const string &publicKeyPath, const string &privateKeyPath);

        /**
         * @brief Gets the current public key
         * @return Constant reference to the public key bytes
         */
        [[nodiscard]] const vector<unsigned char> &getPublicKey() const;

        /**
         * @brief Gets the current private key
         * @return Constant reference to the private key bytes
         */
        [[nodiscard]] const vector<unsigned char> &getPrivateKey() const;

    private:
        vector<unsigned char> publicKey;
        vector<unsigned char> privateKey;

        /**
         * @brief Helper function to write byte data to a file
         * @param path Path to the output file
         * @param data Byte vector to write
         * @return true if write succeeds, false if a file cannot be opened or write fails
         */
        static bool writeFile(const string &path, const vector<unsigned char> &data);

        /**
         * @brief Helper function to read byte data from a file
         * @param path Path to the input file
         * @param data Byte vector where data will be stored
         * @return true if read succeeds, false if a file cannot be opened or read fails
         */
        static bool readFile(const string &path, vector<unsigned char> &data);
    };
}