#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <string>
#include <unordered_map>

bool compressFile(const std::string& inputPath, const std::string& outputPath);
bool decompressFile(const std::string& inputPath, const std::string& outputPath);

#endif // HUFFMAN_H
