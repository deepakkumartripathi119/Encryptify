#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../fileHandling/ReadEnv.cpp"

int executeCryption(const std::string& taskData) {
    Task task = Task::fromString(taskData);
    ReadEnv env;
    std::string envKey = env.getenv();
    int key = std::stoi(envKey);
    if (task.action == Action::ENCRYPT) {
        char ch;
        while (task.f_stream.get(ch)) {
            std::streampos current_pos = task.f_stream.tellg();  
            ch = (ch + key) % 256;
            task.f_stream.seekp(current_pos - (std::streamoff)1);
            task.f_stream.put(ch);
        }
        task.f_stream.close();
    } else {
        char ch;
        while (task.f_stream.get(ch)) {
            std::streampos current_pos = task.f_stream.tellg();
            ch = (ch - key + 256) % 256;
            task.f_stream.seekp(current_pos - (std::streamoff)1);
            task.f_stream.put(ch);
        }
        task.f_stream.close();
    }
    return 0;
}