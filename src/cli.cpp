#include "cli.h"
#include "file_ops.h"
#include <iostream>
#include <sstream>
#include <vector>

std::vector<std::string> split(const std::string &str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string word;
    while (iss >> word) tokens.push_back(word);
    return tokens;
}

void handleCommand(const std::string& input) {
    auto tokens = split(input);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    if (cmd == "add") {
        if (tokens.size() < 2) {
            std::cout << "⚠️ Usage: add <file_path>\n";
        } else {
            addFileToVault(tokens[1]);
        }
    } 
    else if(cmd == "extract"){
        extractFile(tokens[1]); 
    } 
    else if (cmd == "get") {
        std::cout << "📂 Retrieving file...\n";
    }
    else if (cmd == "list") {
        std::cout << "📄 Listing vault contents...\n";
    }
    else if (cmd == "help") {
        std::cout << "Available commands: add <file>, get <file>, list, exit\n";
    }
    else {
        std::cout << "❓ Unknown command: " << cmd << "\n";
    }
}
