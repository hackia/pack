#include "../include/KeyManager.hpp"
#include <fstream>
#include <stdexcept>
#include <sodium.h>
using namespace std;
KeyManager::KeyManager() {
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }
    publicKey.resize(crypto_sign_ed25519_PUBLICKEYBYTES);
    privateKey.resize(crypto_sign_ed25519_SECRETKEYBYTES);
}

bool KeyManager::generateKeys() {
    if (crypto_sign_ed25519_keypair(publicKey.data(), privateKey.data()) != 0) {
        return false;
    }
    return true;
}

bool KeyManager::saveKeys(const std::string &publicKeyPath, const std::string &privateKeyPath) const {
    if (!writeFile(publicKeyPath, publicKey)) {
        return false;
    }
    if (!writeFile(privateKeyPath, privateKey)) {
        return false;
    }
    return true;
}

bool KeyManager::loadKeys(const std::string &publicKeyPath, const std::string &privateKeyPath) {
    if (!readFile(publicKeyPath, publicKey)) {
        return false;
    }
    if (!readFile(privateKeyPath, privateKey)) {
        return false;
    }
    if (publicKey.size() != crypto_sign_ed25519_PUBLICKEYBYTES || privateKey.size() !=
        crypto_sign_ed25519_SECRETKEYBYTES) {
        return false;
    }
    return true;
}

const std::vector<unsigned char> &KeyManager::getPublicKey() const {
    return publicKey;
}

const std::vector<unsigned char> &KeyManager::getPrivateKey() const {
    return privateKey;
}

bool KeyManager::writeFile(const std::string &path, const std::vector<unsigned char> &data) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        return false;
    }
    ofs.write(reinterpret_cast<const char *>(data.data()), static_cast<streamsize>(data.size()));
    return ofs.good();
}

bool KeyManager::readFile(const std::string &path, std::vector<unsigned char> &data) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        return false;
    }
    const std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    data.resize(size);
    ifs.read(reinterpret_cast<char *>(data.data()), size);
    return ifs.good();
}
