#include "trie.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace {
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

std::string normalize(std::string word) {
    for (char& ch : word) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return word;
}
}

TrieNode::TrieNode() : isEndOfWord(false), topK{}, topKSize(0) {
    std::fill(std::begin(children), std::end(children), nullptr);
}

Trie::Trie() : root(new TrieNode()) {}

Trie::~Trie() {
    deleteNodes(root);
}

void Trie::deleteNodes(TrieNode* node) noexcept {
    if (node == nullptr) {
        return;
    }

    for (TrieNode* child : node->children) {
        deleteNodes(child);
    }
    delete node;
}

void Trie::clearTopK(TrieNode* node) {
    if (node == nullptr) {
        return;
    }

    node->topK.fill(0);
    node->topKSize = 0;

    for (TrieNode* child : node->children) {
        clearTopK(child);
    }
}

void Trie::insert(const std::string& word) {
    // Validate the complete word before allocating any nodes.
    if (!isValidWord(word)) {
        return;
    }

    TrieNode* current = root;
    for (char ch : word) {
        const int index = ch - 'a';
        if (current->children[index] == nullptr) {
            current->children[index] = new TrieNode();
        }
        current = current->children[index];
    }
    current->isEndOfWord = true;
}

bool Trie::search(const std::string& word) const {
    if (!isValidWord(word)) {
        return false;
    }

    const TrieNode* current = root;
    for (char ch : word) {
        const int index = ch - 'a';
        if (current->children[index] == nullptr) {
            return false;
        }
        current = current->children[index];
    }
    return current->isEndOfWord;
}

bool Trie::loadDictionary(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string word;
    while (file >> word) {
        word = normalize(word);
        if (isValidWord(word)) {
            insert(word);
        }
    }
    return !file.bad();
}

void Trie::collectWords(TrieNode* node, std::string& currentWord,
                        std::vector<std::string>& results) const {
    if (node->isEndOfWord) {
        results.push_back(currentWord);
    }

    for (int i = 0; i < 26; ++i) {
        if (node->children[i] != nullptr) {
            currentWord.push_back(static_cast<char>('a' + i));
            collectWords(node->children[i], currentWord, results);
            currentWord.pop_back();
        }
    }
}

std::vector<std::string> Trie::getSuggestions(const std::string& prefix) const {
    TrieNode* current = root;
    for (char ch : prefix) {
        if (ch < 'a' || ch > 'z') {
            return {};
        }

        const int index = ch - 'a';
        if (current->children[index] == nullptr) {
            return {};
        }
        current = current->children[index];
    }

    std::vector<std::string> results;
    std::string currentWord = prefix;
    collectWords(current, currentWord, results);
    return results;
}

void Trie::considerCandidate(TrieNode* node, int wordId,
                             const FrequencyMap& frequencyMap) {
    std::array<int, K + 1> candidates{};
    int count = 0;
    bool alreadyPresent = false;

    for (int i = 0; i < node->topKSize; ++i) {
        candidates[count++] = node->topK[static_cast<std::size_t>(i)];
        if (node->topK[static_cast<std::size_t>(i)] == wordId) {
            alreadyPresent = true;
        }
    }

    if (!alreadyPresent) {
        candidates[static_cast<std::size_t>(count++)] = wordId;
    }

    std::sort(candidates.begin(), candidates.begin() + count,
        [&frequencyMap](int a, int b) {
            const int frequencyA = frequencyMap.getFrequency(a);
            const int frequencyB = frequencyMap.getFrequency(b);
            if (frequencyA != frequencyB) {
                return frequencyA > frequencyB;
            }
            return a < b;
        });

    node->topK.fill(0);
    node->topKSize = std::min(count, K);
    for (int i = 0; i < node->topKSize; ++i) {
        node->topK[static_cast<std::size_t>(i)] =
            candidates[static_cast<std::size_t>(i)];
    }
}

void Trie::updateTopK(const std::string& word, int wordId,
                      const FrequencyMap& frequencyMap) {
    if (!isValidWord(word) || wordId <= 0) {
        return;
    }

    TrieNode* current = root;
    for (char ch : word) {
        const int index = ch - 'a';
        current = current->children[index];
        if (current == nullptr) {
            return;
        }
        considerCandidate(current, wordId, frequencyMap);
    }
}

void Trie::rebuildTopK(const FrequencyMap& frequencyMap,
                       const WordTable& wordTable) {
    clearTopK(root);
    const std::vector<std::string>& allWords = wordTable.getAllWords();

    for (std::size_t i = 0; i < allWords.size(); ++i) {
        const std::string& word = allWords[i];
        if (!isValidWord(word)) {
            continue;
        }

        // Ensure each registered word has a complete Trie path.
        insert(word);
        TrieNode* current = root;
        const int wordId = static_cast<int>(i) + 1;

        for (char ch : word) {
            current = current->children[ch - 'a'];
            considerCandidate(current, wordId, frequencyMap);
        }
    }
}

std::vector<int> Trie::getTopK(const std::string& prefix) const {
    TrieNode* current = root;
    for (char ch : prefix) {
        if (ch < 'a' || ch > 'z') {
            return {};
        }

        const int index = ch - 'a';
        if (current->children[index] == nullptr) {
            return {};
        }
        current = current->children[index];
    }

    return std::vector<int>(current->topK.begin(),
                            current->topK.begin() + current->topKSize);
}

TrieNode* Trie::getRoot() const {
    return root;
}

TrieNode* Trie::getChild(TrieNode* node, char ch) const {
    if (node == nullptr) {
        return nullptr;
    }

    if (ch >= 'A' && ch <= 'Z') {
        ch = static_cast<char>(ch - 'A' + 'a');
    }
    if (ch < 'a' || ch > 'z') {
        return nullptr;
    }
    return node->children[ch - 'a'];
}
