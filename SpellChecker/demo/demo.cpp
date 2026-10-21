#include<iostream>
#include "../NgramInvertedIndex.h"
#include <unordered_map>
#include "../Suggestions.h"


std::unordered_map<std::string, std::vector<std::uint32_t>> invIndex;

int main() {
    std::vector<std::string> words;
    std::vector<std::string> sug;
    std::string wrongWord;

    buildNgramInvertedIndex(&invIndex, words, "../dictionary-100k.txt");

    cout<<"Size of words vector in demo : "<<words.size()<<endl;

    cout<<"Size of index: "<<invIndex.size()<<endl;

    cout<<"Enter Word: ";
    cin>>wrongWord;

    sug = getSuggestions(wrongWord, invIndex, words);

    cout<<"Following are the suggestions: "<<endl;
    for(auto& i:sug) {
        cout<<"Suggesetion: "<<i<<endl;
    }

    return 0;
}
