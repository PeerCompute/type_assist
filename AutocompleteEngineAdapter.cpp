
#include "AutocompleteEngineAdapter.h"

#include <cctype>
#include <fstream>
#include <iostream>

bool AutocompleteEngineAdapter::initialize(
    const std::string& dictionaryFile,
    const std::string& frequencyFile)
{
    frequencyFilePath = frequencyFile;

    if (!engine.loadDictionary(dictionaryFile)) {
        return false;
    }

    // Allow the program to start if the frequency file does not exist yet.
    std::ifstream file(frequencyFile);

    if (file.good()) {
        file.close();

        if (!engine.loadFrequencies(frequencyFile)) {
            return false;
        }
    }

    engine.rebuildTopK();
    return true;
}

void AutocompleteEngineAdapter::processInput(char ch)
{
    ch = static_cast<char>(
        std::tolower(static_cast<unsigned char>(ch)));

    if (ch >= 'a' && ch <= 'z') {
        prefix += ch;
    }
}

void AutocompleteEngineAdapter::moveBack()
{
    if (!prefix.empty()) {
        prefix.pop_back();
    }
}

void AutocompleteEngineAdapter::resetToRoot()
{
    prefix.clear();
}

std::vector<std::string>
AutocompleteEngineAdapter::getSuggestions()
{
    return engine.getTopKSuggestions(prefix);
}


void AutocompleteEngineAdapter::updateFrequency(
    const std::string& word)
{
    engine.recordWord(word);

    if (!engine.saveFrequencies(frequencyFilePath)) {
        // Report the failure instead of silently ignoring it.
        std::cerr << "Failed to save frequencies to: "
                  << frequencyFilePath << '\n';
    }
}


void AutocompleteEngineAdapter::addWord(
    const std::string& word)
{
    std::string normalized = word;

    // Convert the word to lowercase.
    for (char& ch : normalized) {
        ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch)));
    }

    // Accept only non-empty words containing a-z.
    if (normalized.empty()) {
        return;
    }

    for (char ch : normalized) {
        if (ch < 'a' || ch > 'z') {
            return;
        }
    }

    // Append the new word to the dictionary file.
    std::ofstream file(dictionaryFilePath, std::ios::app);

    if (!file.is_open()) {
        return;
    }

    file << normalized << '\n';
    file.close();

    // Reload the dictionary so the word becomes available immediately.
    engine.loadDictionary(dictionaryFilePath);
    engine.rebuildTopK();
}
