#pragma once

#include <set>
#include <string>
#include <vector>

namespace ue5parse::conversion {

class Warnings {
public:
    void add(std::string message) {
        if (seen.insert(message).second)
            messages.push_back(std::move(message));
    }

    const std::vector<std::string>& getMessages() const { return messages; }

private:
    std::set<std::string> seen;
    std::vector<std::string> messages;
};

}
