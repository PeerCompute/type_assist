#ifndef DUMMYAUTOCOMPLETEENGINE_H
#define DUMMYAUTOCOMPLETEENGINE_H

#include "iAutocompleteEngine.h"
#include <algorithm>
#include <vector>
#include <string>

// Dummy autocomplete engine for local testing
class DummyAutocompleteEngine : public IAutocompleteEngine {
public:
    void processInput(char ch) override { m_prefix.push_back(ch); }
    void moveBack() override { if (!m_prefix.empty()) m_prefix.pop_back(); }
    void resetToRoot() override { m_prefix.clear(); }

    std::vector<std::string> getSuggestions() override {
        std::vector<std::string> out;
        for (const auto& w : m_words) {
            if (w.rfind(m_prefix, 0) == 0 && w != m_prefix)
                out.push_back(w);
            if (out.size() == 3) break;
        }
        return out;
    }

    void updateFrequency(const std::string&) override {}

    void addWord(const std::string& word) override {
        if (std::find(m_words.begin(), m_words.end(), word) == m_words.end())
            m_words.push_back(word);
    }

private:
    std::string m_prefix;
    std::vector<std::string> m_words{
        "about", "action", "algorithm", "application", "autocomplete",
        "beautiful", "because", "begin", "believe", "build",
        "check", "college", "complete", "computer", "control",
        "database", "development", "different", "document", "device",
        "editor", "engineering", "error", "example", "execute",
        "feature", "file", "format", "function", "future",
        "going", "gone", "good", "government", "great",
        "hardware", "header", "hello", "helper", "highlight",
        "implement", "information", "intelligent", "interface",
        "language", "layout", "learning", "library",
        "network", "normal", "notepad", "number",
        "popup", "prefix", "process", "program", "project",
        "reason", "receive", "recipe", "result", "return",
        "smartwrite", "software", "spellcheck", "student", "suggestion", "system",
        "technology", "test", "text", "that", "the", "their",
        "underline", "university", "update", "user",
        "variable", "version", "view",
        "widget", "width", "window", "without", "world", "write"
    };
};

#endif