#include <iostream>
using namespace std;
#include "NgramInvertedIndex.h"
#include <algorithm>
#include "levinstien+kbrd.h"
#include <stdlib.h>
#include <chrono>

vector<string> getSuggestions(const string& incorrectWord, const unordered_map<string, vector<uint32_t>>& index, const vector<string>& words) {

    vector<string> ngrams;
    vector<string> suggestions;
    unordered_map<uint32_t, int> candFreq;

    getNgrams(incorrectWord, ngrams);

    for (const auto& ngram : ngrams) {
        auto it = index.find(ngram);

        if (it != index.end()) {
            for (const auto& wordIndex : it->second) {
                candFreq[wordIndex]++;
            }
        }
    }

    vector<pair<uint32_t, int>> sortedCandidates(candFreq.begin(), candFreq.end());

    sort(sortedCandidates.begin(), sortedCandidates.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    for (size_t i = 0; i < sortedCandidates.size(); i++) {
        const string& candidate = words[sortedCandidates[i].first];

        size_t candidateLen = candidate.size();
        size_t inputLen = incorrectWord.size();

        size_t diff = (candidateLen > inputLen)
                        ? candidateLen - inputLen
                        : inputLen - candidateLen;

        if (diff <= 3) {
            suggestions.push_back(candidate);
        }
    }

    return recommendation(incorrectWord, suggestions);
}
/*
int main(){
    unordered_map<string, vector<uint32_t>> index;
    vector<string> words;
    buildNgramInvertedIndex(&index, words);
    cout << "Index built successfully.\n";

    cout << "Enter an incorrect word: ";
    string incorrectWord;
    cin >> incorrectWord;
//    incorrectWord = "**" + incorrectWord + "**";

    auto start = std::chrono::high_resolution_clock::now();
    vector<string> suggestions = getSuggestions(incorrectWord, index, words);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for sggestion: " << duration.count() << " ms" << std::endl;

    cout << "Suggestions:\n";
    for (const auto& suggestion : suggestions) {
        cout << suggestion << endl;
    }
    return 0;
}*/
