#include "file_ops.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <random>
#include <iomanip>
#include <sstream>
#include <map>
#include "json.hpp"
#include "huffman.h"
#include "encryption.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void addFileToVault(const std::string& filepath) {
    if (!fs::exists("vault_index.json") || fs::is_empty("vault_index.json")) {
        std::ofstream initFile("vault_index.json");
        initFile << "{}";
        initFile.close();
    }

    fs::path sourcePath(filepath);

    if (!fs::exists(sourcePath)) {
        std::cout << "❌ File not found: " << filepath << "\n";
        return;
    }

    // Read source file
    std::ifstream inFile(sourcePath, std::ios::binary);
    if (!inFile) {
        std::cout << "❌ Failed to open file.\n";
        return;
    }

    // Generate UUID
    std::string uuid = generateUUID();

    // Copy file to vault_data
    fs::path vaultDir = "vault_data";
    fs::create_directories(vaultDir);
    fs::path targetPath = vaultDir / uuid;

    inFile.close();

    bool useCompression = true;

    std::string tempCompressed = "temp_compressed.bin";
    if (!compressFile(filepath, tempCompressed)) {
        std::cerr << "Compression failed.\n";
        return;
    }

    //Check if compression is actually smaller
    auto originalSize = fs::file_size(filepath);
    auto compressedSize = fs::file_size(tempCompressed);

    if(compressedSize >= originalSize){
        std::cout<< "i Compression not efficient, storing uncompressed. \n";
        std::remove(tempCompressed.c_str());
        fs::copy(filepath, tempCompressed);
        useCompression = false;
    }
    if(!useCompression){
        compressedSize = originalSize;
    }
    
    std::cout << "✅ Compressed " << "\n";
    std::cout << "Original size: " << originalSize << " bytes\n";
    std::cout << "Compressed size: " << compressedSize << " bytes\n";
    
    if (originalSize > 0) {
        double ratio = (1.0 - (double)compressedSize / originalSize) * 100.0;
        std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) << ratio << "%\n";
    }

    std::string password = getPasswordFromUser(); // New: get password from user

    // New: call encryptFile with the password
    if (!encryptFile(tempCompressed, targetPath.string(), password)) {
        std::cerr << "Encryption failed.\n";
        return;
    }
    std::remove(tempCompressed.c_str());

    std::cout << "✅ File added to vault with ID: " << uuid << "\n";

    // Load existing index
    json index;
    std::ifstream indexFile("vault_index.json");
    if (indexFile) {
        std::stringstream buffer;
        buffer << indexFile.rdbuf();
        std::string content = buffer.str();
        if (!content.empty()) {
            index = json::parse(content);
        }
        indexFile.close();
    }


    // Add entry
    index[filepath] = {
        {"uuid", uuid},
        {"compressed", useCompression}  
    };

    // Save back
    std::ofstream outIndex("vault_index.json");
    outIndex << std::setw(4) << index;
    outIndex.close();
}


void extractFile(const std::string& filename) {
    std::ifstream indexFile("vault_index.json");
    if (!indexFile) {
        std::cout << "❌ Index file not found.\n";
        return;
    }

    json index;
    indexFile >> index;

    if (!index.contains(filename)) {
        std::cout << "❌ File not found in vault: " << filename << "\n";
        return;
    }

    bool isCompressed = index[filename]["compressed"];
    std::string uuid = index[filename]["uuid"];
    fs::path vaultPath = fs::path("vault_data") / uuid;

    if (!fs::exists(vaultPath)) {
        std::cout << "❌ Encrypted file missing: " << vaultPath << "\n";
        return;
    }

    //Step 1: Decrypt into a temporary file
    std::string tempDecrypted = "temp_decrypted.bin";
    std::string password = getPasswordFromUser(); // New: get password from user

    // New: call decryptFile with the password
    if(!decryptFile(vaultPath.string() , tempDecrypted , password)){
        std::cerr << "❌ Decryption failed. (Incorrect password?)\n";
        return;
    }
    //Step 2: Decompress the decrypted file into final output
    fs::path outputPath = "extracted_" + fs::path(filename).filename().string();
    if (isCompressed) {
        if (!decompressFile(tempDecrypted, outputPath.string())) {
            std::cerr << "❌ Decompression failed.\n";
            std::remove(tempDecrypted.c_str());
            return;
        }
    } else {
        fs::copy(tempDecrypted, outputPath, fs::copy_options::overwrite_existing);
    }

    // Cleanup
    std::remove(tempDecrypted.c_str());

    std::cout << "✅ File extracted to: " << outputPath << "\n";
}
