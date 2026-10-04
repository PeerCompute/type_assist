#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <vector>

using namespace std;

class TrieNode {
public:
    TrieNode* children[26];
    bool isEndOfWord;

    TrieNode();
};

class Trie {
private:
    TrieNode* root;

    void collectWords(TrieNode* node, string currentWord, vector<string>& suggestions);

public:
    Trie();

    void insert(const string& word);
    bool search(const string& word);
    vector<string> getSuggestions(const string& prefix);
};

#endif