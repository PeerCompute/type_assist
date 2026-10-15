#include "wordTable.h"

int WordTable::getOrCreateId(const std::string& word) {
    const auto it = wordToId.find(word);
    if (it != wordToId.end()) {
        return it->second;
    }

    const int id = static_cast<int>(idToWord.size()) + 1;
    wordToId.emplace(word, id);
    idToWord.push_back(word);
    return id;
}

int WordTable::getId(const std::string& word) const {
    const auto it = wordToId.find(word);
    return it == wordToId.end() ? 0 : it->second;
}

std::string WordTable::getWord(int id) const {
    if (id < 1 || id > static_cast<int>(idToWord.size())) {
        return {};
    }
    return idToWord[static_cast<std::size_t>(id - 1)];
}

const std::vector<std::string>& WordTable::getAllWords() const {
    return idToWord;
}
