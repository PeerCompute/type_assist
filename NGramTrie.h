#ifndef NGRAMTRIE_H
#define NGRAMTRIE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "wordTable.h"

using namespace std;

class NGramTrie
{
private:

    struct NGramNode
    {
        unordered_map<int, unique_ptr<NGramNode>> children;

        int frequency = 0;       
        int userFrequency = 0;   

        vector<pair<int, int>> topK;
    };

    NGramNode root;

    static const int MAX_DEPTH = 3;
    static const int K = 3;

    void insertSentence(
        const vector<string>& words,
        WordTable& wordTable
    );

    void refreshNode(
        NGramNode* node,
        const WordTable& wordTable
    );

    void buildCache(
        NGramNode* node,
        const WordTable& wordTable
    );

    vector<pair<int, int>> getCandidates(
        const NGramNode* node,
        const string& prefix,
        const WordTable& wordTable,
        int k
    ) const;

public:

    void addSentence(
        const vector<string>& words,
        WordTable& wordTable
    );

    void buildTopK(const WordTable& wordTable);

    // Learns one word and refreshes only the affected Top K caches.
    void observeWord(
        const vector<string>& previousWords,
        const string& word,
        WordTable& wordTable
    );

    vector<pair<string, int>> predict(
        const vector<string>& previousWords,
        const string& prefix,
        const WordTable& wordTable,
        int k = 3
    ) const;
};

#endif