#ifndef TRIE_H
#define TRIE_H

#include "frequencyMap.h"
#include "wordTable.h"

#include <array>
#include <string>
#include <vector>

constexpr int K = 3;

class TrieNode {
public:
    TrieNode* children[26];
    bool isEndOfWord;
    std::array<int, K> topK{};
    int topKSize = 0;

    TrieNode();
};

class Trie {
private:
    TrieNode* root;

    void deleteNodes(TrieNode* node) noexcept;
    void clearTopK(TrieNode* node);
    void collectWords(TrieNode* node, std::string& currentWord,
                      std::vector<std::string>& results) const;
    void considerCandidate(TrieNode* node, int wordId,
                           const FrequencyMap& frequencyMap);

public:
    Trie();
    ~Trie();

    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;
    Trie(Trie&&) = delete;
    Trie& operator=(Trie&&) = delete;

    void insert(const std::string& word);
    bool search(const std::string& word) const;
    bool loadDictionary(const std::string& filename);
    std::vector<std::string> getSuggestions(const std::string& prefix) const;

    void updateTopK(const std::string& word, int wordId,
                    const FrequencyMap& frequencyMap);
    void rebuildTopK(const FrequencyMap& frequencyMap,
                     const WordTable& wordTable);
    std::vector<int> getTopK(const std::string& prefix) const;

    TrieNode* getRoot() const;
    TrieNode* getChild(TrieNode* node, char ch) const;
};

#endif
