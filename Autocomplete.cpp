#include "Autocomplete.h"

#include <cctype>
#include <fstream>
#include <iostream>

namespace {
std::string normalize(std::string word) {
    for (char& ch : word) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return word;
}

bool isValidWord(const std::string& word) {
    if (word.empty()) {
        return false;
    }

    for (char ch : word) {
        if (ch < 'a' || ch > 'z') {
            return false;
        }
    }
    return true;
}

bool isValidPrefix(const std::string& prefix) {
    for (char ch : prefix) {
        if (ch < 'a' || ch > 'z') {
            return false;
        }
    }
    return true;
}
}

bool Autocomplete::loadDictionary(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open dictionary file: " << filename << '\n';
        return false;
    }

    // Invalidate cached rankings before changing the vocabulary.
    topKReady = false;
    std::string word;
    while (file >> word) {
        word = normalize(word);
        if (!isValidWord(word)) {
            continue;
        }

        wordTable.getOrCreateId(word);
        trie.insert(word);
    }

    return !file.bad();
}

std::vector<std::string> Autocomplete::getTopKSuggestions(
    const std::string& input) {
    const std::string prefix = normalize(input);
    if (!isValidPrefix(prefix)) {
        return {};
    }
    if (!topKReady) {
        rebuildTopK();
    }

    const std::vector<int> ids = trie.getTopK(prefix);
    std::vector<std::string> suggestions;
    suggestions.reserve(ids.size());

    for (int id : ids) {
        const std::string word = wordTable.getWord(id);
        if (!word.empty()) {
            suggestions.push_back(word);
        }
    }
    return suggestions;
}

std::vector<std::string> Autocomplete::getTopKSuggestionsAtNode(
    const TrieNode* node) const {
    std::vector<std::string> suggestions;
    if (node == nullptr) {
        return suggestions;
    }

    suggestions.reserve(static_cast<std::size_t>(node->topKSize));
    for (int i = 0; i < node->topKSize; ++i) {
        const std::string word = node->topK[static_cast<std::size_t>(i)] == 0
            ? std::string{}
            : wordTable.getWord(node->topK[static_cast<std::size_t>(i)]);
        if (!word.empty()) {
            suggestions.push_back(word);
        }
    }
    return suggestions;
}

void Autocomplete::recordWord(const std::string& input) {
    const std::string word = normalize(input);
    if (!isValidWord(word)) {
        return;
    }

    const int id = wordTable.getOrCreateId(word);
    if (!trie.search(word)) {
        trie.insert(word);
    }

    frequencyMap.setFrequency(id, frequencyMap.getFrequency(id) + 1);

    if (topKReady) {
        trie.updateTopK(word, id, frequencyMap);
    } else {
        // Build a correct cache if recordWord is used before explicit setup.
        rebuildTopK();
    }
}

bool Autocomplete::loadFrequencies(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // A partially read file must not leave rankings marked as current.
    topKReady = false;
    std::string word;
    while (file >> word) {
        int count = 0;
        if (!(file >> count)) {
            return false;
        }

        word = normalize(word);
        if (!isValidWord(word) || count < 0) {
            continue;
        }

        const int id = wordTable.getOrCreateId(word);
        frequencyMap.setFrequency(id, count);
        trie.insert(word);
    }

    return !file.bad();
}

bool Autocomplete::saveFrequencies(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    const std::vector<std::string>& words = wordTable.getAllWords();
    for (std::size_t i = 0; i < words.size(); ++i) {
        const int id = static_cast<int>(i) + 1;
        const int count = frequencyMap.getFrequency(id);

        // Missing counts default to zero; only persist words actually used.
        if (count > 0) {
            file << words[i] << ' ' << count << '\n';
        }
    }

    file.close();
    return !file.fail();
}

void Autocomplete::rebuildTopK() {
    trie.rebuildTopK(frequencyMap, wordTable);
    topKReady = true;
}

int Autocomplete::getFrequency(const std::string& input) const {
    const std::string word = normalize(input);
    if (!isValidWord(word)) {
        return 0;
    }

    const int id = wordTable.getId(word);
    return id == 0 ? 0 : frequencyMap.getFrequency(id);
}

Trie& Autocomplete::getTrie() {
    return trie;
}
