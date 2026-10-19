
#ifndef AUTOCOMPLETE_ENGINE_ADAPTER_H
#define AUTOCOMPLETE_ENGINE_ADAPTER_H

#include "iAutocompleteEngine.h"
#include "Autocomplete.h"

#include <string>
#include <vector>

class AutocompleteEngineAdapter : public IAutocompleteEngine {
private:
    Autocomplete engine;
    std::string prefix;
    std::string frequencyFilePath;
    std::string dictionaryFilePath;
public:
    bool initialize(const std::string& dictionaryFile,
                    const std::string& frequencyFile);

    void processInput(char ch) override;
    void moveBack() override;
    void resetToRoot() override;

    std::vector<std::string> getSuggestions() override;

    void updateFrequency(const std::string& word) override;
    void addWord(const std::string& word) override;
};

#endif
