#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

// Function to securely read a password from the command line
std::string getPasswordFromUser();

// These functions now take a user-provided password
bool encryptFile(const std::string &inputPath, const std::string &outputPath, const std::string &password);
bool decryptFile(const std::string &inputPath, const std::string &outputPath, const std::string &password);

#endif // ENCRYPTION_H