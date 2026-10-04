#include "Trie.h"

TrieNode::TrieNode() {
    isEndOfWord = false;

    for (int i = 0; i < 26; i++) {
        children[i] = nullptr;
    }
}

Trie::Trie() {
    root = new TrieNode();
}
void Trie::insert(const string& word) {
    TrieNode* current = root;

    for (char ch : word) {
        int index = ch - 'a';

        if (current->children[index] == nullptr) {
            current->children[index] = new TrieNode();
        }

        current = current->children[index];
    }

    current->isEndOfWord = true;
}
bool Trie::search(const string& word) {
    TrieNode* current = root;

    for (char ch : word) {
        int index = ch - 'a';

        if (current->children[index] == nullptr) {
            return false;
        }

        current = current->children[index];
    }

    return current->isEndOfWord;
}

void Trie::collectWords(TrieNode* node, string currentWord, vector<string>& suggestions) {
    if (node->isEndOfWord) {
        suggestions.push_back(currentWord);
    }

    for (int i = 0; i < 26; i++) {
        if (node->children[i] != nullptr) {
            char nextChar = 'a' + i;
            collectWords(node->children[i], currentWord + nextChar, suggestions);
        }
    }
}
vector<string> Trie::getSuggestions(const string& prefix) {
    vector<string> suggestions;

    TrieNode* current = root;

    for (char ch : prefix) {
        int index = ch - 'a';

        if (current->children[index] == nullptr) {
            return suggestions;
        }

        current = current->children[index];
    }

    collectWords(current, prefix, suggestions);

    return suggestions;
}