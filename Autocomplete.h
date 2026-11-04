#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <string>
#include <vector>
#include <utility>

#include "wordTable.h"
#include "NGramTrie.h"

using namespace std;

class Autocomplete
{
private:

    WordTable wordTable;
    NGramTrie ngramTrie;

    vector<string> tokenize(const string& text) const;

public:

    bool trainFromFile(const string& filename);

    vector<pair<string, int>> getSuggestions(
        const string& input,
        int k = 3
    ) const;

    void learnFromCompletedInput(const string& input);
};

#endif