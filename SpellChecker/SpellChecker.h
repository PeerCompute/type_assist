#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include "../ispellchecker.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include "NgramInvertedIndex.h"
#include "Suggestions.h"
#include <iostream>


class SpellChecker : public ISpellChecker {
public:
        CheckResult checkWord(const std::string& word, int k) override {
        std::string low = word;
        for (char& c : low)
            c = std::tolower(static_cast<unsigned char>(c));

        auto it = std::find(m_addedWords.begin(), m_addedWords.end(), low);
        if (it != m_addedWords.end())
            return { true, {} };
/*
        auto it = m_typos.find(low);
        if (it != m_typos.end()) {
            std::vector<std::string> sugs = it->second;
            if (!word.empty() && std::isupper(static_cast<unsigned char>(word[0]))) {
                for (auto& s : sugs) {
                    if (!s.empty()) s[0] = std::toupper(static_cast<unsigned char>(s[0]));
                }
            }
            return { false, sugs };
        }*/

        cout<<"Just before getSuggestions"<<endl;
        std::vector<std::string> sug = getSuggestions(word, invIndex, words);

        return {false, sug};
//        return {false, {"one", "two", "three"}};

        //return { true, {} };
    }

    void addToDictionary(const std::string& word) override {
        std::string low = word;
        for (char& c : low)
            c = std::tolower(static_cast<unsigned char>(c));
        m_addedWords.push_back(low);
    }

    int initialize(std::string dictName) {
        if(buildNgramInvertedIndex(&invIndex, words, dictName)) {
            return 0;
        }
        m_addedWords = words;
        return 1;
    }

    std::unordered_map<std::string, std::vector<std::uint32_t>> invIndex;
    std::vector<std::string> words;
    std::unordered_map<std::string, std::vector<std::string>> m_typos;
//    std::unordered_set<std::string> m_addedWords;
    std::vector<std::string> m_addedWords;
};

#endif
