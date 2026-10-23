#ifndef IAUTOCOMPLETEENGINE_H
#define IAUTOCOMPLETEENGINE_H

#include <string>
#include <vector>

class IAutocompleteEngine {
public:
    virtual ~IAutocompleteEngine() = default;

    // Move one step down the Trie for this letter (always lower-case a-z)
    virtual void processInput(char ch) = 0;

    // Move one step back up (backspace)
    virtual void moveBack() = 0;

    // Go back to the root (word finished or cursor moved)
    virtual void resetToRoot() = 0;

    // Top-K words for the current position, as strings
    virtual std::vector<std::string> getSuggestions() = 0;

    // Called when a word is completed or selected
    virtual void updateFrequency(const std::string& word) = 0;

    // Called when the user adds a new word to the dictionary
    virtual void addWord(const std::string& word) = 0;
};

#endif