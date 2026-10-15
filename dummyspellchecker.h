#ifndef DUMMYSPELLCHECKER_H
#define DUMMYSPELLCHECKER_H

#include "ispellchecker.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cctype>

class DummySpellChecker : public ISpellChecker {
public:
    DummySpellChecker() {
        m_typos = {
            {"payad", {"Payada", "Payal"}},
            {"goin", {"going", "gone", "go"}},
            {"recieve", {"receive", "receipt", "receiver"}},
            {"teh", {"the", "tea", "ten"}},
            {"wrold", {"world", "would", "word"}},
            {"helo", {"hello", "help", "hero"}},
            {"computr", {"computer", "compute", "compact"}},
            {"thier", {"their", "there", "tier"}},
            {"laagging", {"lagging", "flagging", "logging"}},
            {"writting", {"writing", "written", "waiting"}},
            {"definately", {"definitely", "defiantly", "defined"}},
            {"seperate", {"separate", "desperate", "operate"}},
            {"untill", {"until", "units", "untie"}},
            {"beleive", {"believe", "believer", "relief"}},
            {"becuase", {"because", "became", "base"}},
            {"notepd", {"notepad", "noted", "notebook"}},
            {"editer", {"editor", "edited", "editing"}},
            {"projeckt", {"project", "protect", "projector"}},
            {"clg", {"college", "clip", "clear"}},
            {"studnt", {"student", "studied", "studio"}},
            {"sugestion", {"suggestion", "suggested", "suggest"}},
            {"cheker", {"checker", "checked", "check"}}
        };
    }

    CheckResult checkWord(const std::string& word, int) override {
        std::string low = word;
        for (char& c : low)
            c = std::tolower(static_cast<unsigned char>(c));

        if (m_addedWords.find(low) != m_addedWords.end())
            return { true, {} };

        auto it = m_typos.find(low);
        if (it != m_typos.end()) {
            std::vector<std::string> sugs = it->second;
            if (!word.empty() && std::isupper(static_cast<unsigned char>(word[0]))) {
                for (auto& s : sugs) {
                    if (!s.empty()) s[0] = std::toupper(static_cast<unsigned char>(s[0]));
                }
            }
            return { false, sugs };
        }

        return { true, {} };
    }

    void addToDictionary(const std::string& word) override {
        std::string low = word;
        for (char& c : low)
            c = std::tolower(static_cast<unsigned char>(c));
        m_addedWords.insert(low);
    }

private:
    std::unordered_map<std::string, std::vector<std::string>> m_typos;
    std::unordered_set<std::string> m_addedWords;
};

#endif