#include <iostream>
using namespace std;
#include "NgramInvertedIndex.h"
#include <algorithm>

vector<string> getSuggestions(const string& incorrectWord, const unordered_map<string, vector<uint16_t>>& index, const vector<string>& words) {

    vector<string> ngrams;
    vector<string> suggestions;
    unordered_map<uint16_t, int> candFreq;

    getNgrams(incorrectWord, ngrams);

    for (const auto& ngram : ngrams) {
        auto it = index.find(ngram);

        if (it != index.end()) {
            for (const auto& wordIndex : it->second) {
                candFreq[wordIndex]++;
            }
        }
    }

    vector<pair<uint16_t, int>> sortedCandidates(
        candFreq.begin(), candFreq.end()
    );

    sort(sortedCandidates.begin(), sortedCandidates.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    for (size_t i = 0; i < min(sortedCandidates.size(), size_t(5)); i++) {
        suggestions.push_back(words[sortedCandidates[i].first]);
    }

    return suggestions;
}

int main(){
    unordered_map<string, vector<uint16_t>> index;
    vector<string> words;
    buildNgramInvertedIndex(&index, words);
    cout << "Index built successfully.\n";

    cout << "Enter an incorrect word: ";
    string incorrectWord;
    cin >> incorrectWord;
    vector<string> suggestions = getSuggestions(incorrectWord, index, words);

    cout << "Suggestions:\n";
    for (const auto& suggestion : suggestions) {
        cout << suggestion << endl;
    }
    return 0;
}