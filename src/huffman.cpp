#include "huffman.h"
#include <queue>
#include <fstream>
#include <bitset>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

// Node structure for the Huffman tree
struct HuffmanNode {
    char ch;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

// Comparator for priority queue
struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

// Clean up tree memory
void deleteTree(HuffmanNode* node) {
    if (!node) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

// Recursively build the code map
void buildCodes(HuffmanNode* node, const std::string& code, std::unordered_map<char, std::string>& codes) {
    if (!node) return;
    if (!node->left && !node->right) {
        // Handle single character case - assign "0" if code is empty
        codes[node->ch] = code.empty() ? "0" : code;
        return;
    }
    buildCodes(node->left, code + "0", codes);
    buildCodes(node->right, code + "1", codes);
}

// Build Huffman tree from frequency map
HuffmanNode* buildTree(const std::unordered_map<char, int>& freqMap) {
    if (freqMap.empty()) return nullptr;
    
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;

    for (const auto& [ch, freq] : freqMap) {
        pq.push(new HuffmanNode(ch, freq));
    }

    // Handle single character case
    if (pq.size() == 1) {
        HuffmanNode* single = pq.top();
        HuffmanNode* root = new HuffmanNode('\0', single->freq);
        root->left = single;
        return root;
    }

    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();
        HuffmanNode* merged = new HuffmanNode('\0', left->freq + right->freq);
        merged->left = left;
        merged->right = right;
        pq.push(merged);
    }

    return pq.top();
}

// Serialize the tree using preorder traversal
void serializeTree(HuffmanNode* node, std::string& treeBits) {
    if (!node) return;

    if (!node->left && !node->right) {
        treeBits += '1';
        std::bitset<8> charBits(static_cast<unsigned char>(node->ch));
        treeBits += charBits.to_string();
    } else {
        treeBits += '0';
        serializeTree(node->left, treeBits);
        serializeTree(node->right, treeBits);
    }
}

// Deserialize tree from bitstream
HuffmanNode* deserializeTree(std::istream& in) {
    char bit;
    in.get(bit);
    
    if (bit == '1') {
        // Leaf node - read 8 bits for character
        std::bitset<8> bs;
        for (int i = 7; i >= 0; --i) {  // Fixed: correct bit ordering
            char b;
            in.get(b);
            bs[i] = (b == '1') ? 1 : 0;
        }
        return new HuffmanNode(static_cast<char>(bs.to_ulong()), 0);
    } else if (bit == '0') {
        // Internal node
        HuffmanNode* node = new HuffmanNode('\0', 0);
        node->left = deserializeTree(in);
        node->right = deserializeTree(in);
        return node;
    }
    
    return nullptr; // Error case
}

// Encode the input file
bool compressFile(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        std::cerr << "❌ Could not open file for compression.\n";
        return false;
    }

    std::unordered_map<char, int> freqMap;
    std::string data;

    // Read entire file and count frequencies
    char ch;
    while (in.get(ch)) {
        freqMap[ch]++;
        data += ch;
    }
    in.close();

    // Handle empty file
    if (data.empty()) {
        std::ofstream out(outputPath, std::ios::binary);
        out.close();
        std::cout << "✅ Compressed empty file to: " << outputPath << "\n";
        return true;
    }

    // Build Huffman tree
    HuffmanNode* root = buildTree(freqMap);
    if (!root) {
        std::cerr << "❌ Failed to build Huffman tree.\n";
        return false;
    }

    // Build codes map
    std::unordered_map<char, std::string> codes;
    buildCodes(root, "", codes);

    // Encode the data
    std::string encoded;
    for (char c : data) {
        encoded += codes[c];
    }

    // Serialize tree
    std::string treeBits;
    serializeTree(root, treeBits);

    // Write to output file
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        std::cerr << "❌ Could not open output file for writing.\n";
        deleteTree(root);
        return false;
    }

    // Write tree serialization
    out << treeBits << '\n';
    
    // Write bit count (important for handling padding)
    out << encoded.size() << '\n';
    
    // Write encoded data in bytes
    size_t bitsWritten = 0;
    while (bitsWritten < encoded.size()) {
        std::string chunk = encoded.substr(bitsWritten, 8);
        // Pad with zeros if less than 8 bits
        if (chunk.size() < 8) {
            chunk.append(8 - chunk.size(), '0');
        }
        std::bitset<8> b(chunk);
        out.put(static_cast<unsigned char>(b.to_ulong()));
        bitsWritten += 8;
    }

    out.close();
    
    // Clean up memory
    deleteTree(root);
    
    // Display compression statistics
    std::ifstream fin(inputPath, std::ios::binary | std::ios::ate);
    std::ifstream fout(outputPath, std::ios::binary | std::ios::ate);
    
    // auto originalSize = fin.tellg();
    // auto compressedSize = fout.tellg();
    
    fin.close();
    fout.close();
    
    // std::cout << "✅ Compressed to: " << outputPath << "\n";
    // std::cout << "Original size: " << originalSize << " bytes\n";
    // std::cout << "Compressed size: " << compressedSize << " bytes\n";
    
    // if (originalSize > 0) {
    //     double ratio = (1.0 - (double)compressedSize / originalSize) * 100.0;
    //     std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) << ratio << "%\n";
    // }

    return true;
}

// Decode the encoded file
bool decompressFile(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        std::cerr << "❌ Could not open file for decompression.\n";
        return false;
    }

    // Read tree serialization
    std::string treeBits;
    if (!std::getline(in, treeBits)) {
        std::cerr << "❌ Could not read tree data.\n";
        return false;
    }

    // Read bit count
    std::string bitCountStr;
    if (!std::getline(in, bitCountStr)) {
        std::cerr << "❌ Could not read bit count.\n";
        return false;
    }
    
    size_t bitCount = std::stoull(bitCountStr);

    // Handle empty file case
    if (bitCount == 0) {
        std::ofstream out(outputPath, std::ios::binary);
        out.close();
        std::cout << "✅ Decompressed empty file to: " << outputPath << "\n";
        return true;
    }

    // Deserialize the tree
    std::istringstream treeStream(treeBits);
    HuffmanNode* root = deserializeTree(treeStream);
    
    if (!root) {
        std::cerr << "❌ Failed to deserialize Huffman tree.\n";
        return false;
    }

    // Read remaining file data and convert to bit stream
    std::string bitStream;
    char byte;
    while (in.get(byte)) {
        std::bitset<8> b(static_cast<unsigned char>(byte));
        bitStream += b.to_string();
    }
    in.close();

    // Truncate to actual bit count (remove padding)
    if (bitStream.size() >= bitCount) {
        bitStream = bitStream.substr(0, bitCount);
    } else {
        std::cerr << "❌ Insufficient data in compressed file.\n";
        deleteTree(root);
        return false;
    }

    // Decode the bit stream
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        std::cerr << "❌ Could not open output file for writing.\n";
        deleteTree(root);
        return false;
    }

    HuffmanNode* curr = root;
    for (char bit : bitStream) {
        if (bit == '0') {
            curr = curr->left;
        } else if (bit == '1') {
            curr = curr->right;
        } else {
            std::cerr << "❌ Invalid bit in stream.\n";
            deleteTree(root);
            return false;
        }
        
        if (!curr) {
            std::cerr << "❌ Invalid path in Huffman tree.\n";
            deleteTree(root);
            return false;
        }

        // If we reached a leaf node
        if (!curr->left && !curr->right) {
            out.put(curr->ch);
            curr = root; // Reset to root for next character
        }
    }

    out.close();
    deleteTree(root);
    
    std::cout << "✅ Decompressed to: " << outputPath << "\n";
    return true;
}