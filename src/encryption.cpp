#include "encryption.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sodium.h>
#include <conio.h> 

// A cross-platform way to get a password without echoing it to the console
std::string getPasswordFromUser() {
    std::cout << "🔑 Enter password: ";
    std::string password;
    char ch;
    while ((ch = _getch()) != '\r' && ch != '\n') {
        if (ch == '\b') { // Handle backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else {
            password.push_back(ch);
            std::cout << '*';
        }
    }
    std::cout << std::endl;
    return password;
}

bool encryptFile(const std::string &inputPath, const std::string &outputPath, const std::string &password) {
    if (sodium_init() < 0) {
        std::cerr << "❌ libsodium initialization failed.\n";
        return false;
    }

    std::ifstream inFile(inputPath, std::ios::binary);
    std::ofstream outFile(outputPath, std::ios::binary);

    if (!inFile || !outFile) return false;

    unsigned char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof(salt));

    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    if (crypto_pwhash(key, sizeof(key), password.c_str(), password.size(), salt,
                      crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        std::cerr << "❌ Key derivation failed.\n";
        return false;
    }

    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

    crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);

    // Write the salt and header to the output file
    outFile.write(reinterpret_cast<char*>(salt), sizeof(salt));
    outFile.write(reinterpret_cast<char*>(header), sizeof(header));

    const size_t CHUNK_SIZE = 4096;
    std::vector<unsigned char> in_buf(CHUNK_SIZE);
    std::vector<unsigned char> out_buf(CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES);

    while (inFile.read(reinterpret_cast<char*>(in_buf.data()), in_buf.size())) {
        unsigned long long out_len;
        crypto_secretstream_xchacha20poly1305_push(&state, out_buf.data(), &out_len,
                                                 in_buf.data(), inFile.gcount(), NULL, 0, 0);
        outFile.write(reinterpret_cast<char*>(out_buf.data()), out_len);
    }

    unsigned long long out_len;
    crypto_secretstream_xchacha20poly1305_push(&state, out_buf.data(), &out_len,
                                             in_buf.data(), inFile.gcount(), NULL, 0,
                                             crypto_secretstream_xchacha20poly1305_TAG_FINAL);
    outFile.write(reinterpret_cast<char*>(out_buf.data()), out_len);

    std::cout << "✅ File encrypted with a strong KDF and cipher.\n";
    return true;
}

bool decryptFile(const std::string &inputPath, const std::string &outputPath, const std::string &password) {
    if (sodium_init() < 0) {
        std::cerr << "❌ libsodium initialization failed.\n";
        return false;
    }

    std::ifstream inFile(inputPath, std::ios::binary);
    std::ofstream outFile(outputPath, std::ios::binary);

    if (!inFile || !outFile) return false;

    // Read the salt from the beginning of the file
    unsigned char salt[crypto_pwhash_SALTBYTES];
    inFile.read(reinterpret_cast<char*>(salt), sizeof(salt));
    if (inFile.gcount() != sizeof(salt)) {
        std::cerr << "❌ Failed to read salt.\n";
        return false;
    }

    // Read the header from the file
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    inFile.read(reinterpret_cast<char*>(header), sizeof(header));
    if (inFile.gcount() != sizeof(header)) {
        std::cerr << "❌ Failed to read header.\n";
        return false;
    }

    // Derive the key again using the same password and salt.
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    if (crypto_pwhash(key, sizeof(key), password.c_str(), password.size(), salt,
                      crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        std::cerr << "❌ Key derivation failed. (Incorrect password?)\n";
        return false;
    }

    crypto_secretstream_xchacha20poly1305_state state;
    if (crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key) != 0) {
        std::cerr << "❌ Invalid header or key. (Incorrect password?)\n";
        return false;
    }

    const size_t CHUNK_SIZE = 4096;
    std::vector<unsigned char> in_buf(CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES);
    std::vector<unsigned char> out_buf(CHUNK_SIZE);

    while (inFile) {
        inFile.read(reinterpret_cast<char*>(in_buf.data()), in_buf.size());
        if (inFile.gcount() == 0) break;
        
        unsigned long long out_len;
        unsigned char tag;
        if (crypto_secretstream_xchacha20poly1305_pull(&state, out_buf.data(), &out_len, &tag,
                                                  in_buf.data(), inFile.gcount(), NULL, 0) != 0) {
            std::cerr << "❌ Decryption failed. (Corrupted file or incorrect password?)\n";
            return false;
        }
        
        outFile.write(reinterpret_cast<char*>(out_buf.data()), out_len);
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) break;
    }

    std::cout << "✅ File decrypted.\n";
    return true;
}
