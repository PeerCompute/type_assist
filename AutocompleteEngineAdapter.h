#ifndef AUTOCOMPLETE_ENGINE_ADAPTER_H
#define AUTOCOMPLETE_ENGINE_ADAPTER_H

#include "iAutocompleteEngine.h"

#include <memory>
#include <string>
#include <vector>

class Autocomplete;   // N-gram engine (Autocomplete.h, included in the .cpp)

// Connects the editor (IAutocompleteEngine) to the N-gram autocomplete model.
class AutocompleteEngineAdapter : public IAutocompleteEngine {
private:
    std::unique_ptr<Autocomplete> engine;
    std::string prefix;    // only used if no context was supplied
    std::string context;   // sentence before the cursor

public:
    AutocompleteEngineAdapter();
    ~AutocompleteEngineAdapter() override;

    // Trains the model from a corpus text file (one sentence per line).
    bool initialize(const std::string& corpusFile);

    void processInput(char ch) override;
    void moveBack() override;
    void resetToRoot() override;

    void setContext(const std::string& textBeforeCursor) override;
    std::vector<std::string> getSuggestions() override;

    void updateFrequency(const std::string& word) override;
    void addWord(const std::string& word) override;
};

#endif
