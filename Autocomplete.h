#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include "trie.h"
#include "frequencyMap.h"
#include "wordTable.h"

#include <string>
#include <vector>

class Autocomplete {
private:
    Trie trie;
    FrequencyMap frequencyMap;
    WordTable wordTable;
    bool topKReady = false;

public:
    bool loadDictionary(const std::string& filename);
    void recordWord(const std::string& word);
    bool loadFrequencies(const std::string& filename);
    bool saveFrequencies(const std::string& filename) const;
    void rebuildTopK();

    int getFrequency(const std::string& word) const;
    std::vector<std::string> getTopKSuggestions(const std::string& prefix);
    std::vector<std::string> getTopKSuggestionsAtNode(const TrieNode* node) const;

    Trie& getTrie();
};

#endif
