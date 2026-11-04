#include "AutocompleteEngineAdapter.h"
#include "Autocomplete.h"

#include <cctype>

namespace {
std::string lowerAscii(std::string s)
{
    for (char& ch : s)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

bool allLetters(const std::string& s)
{
    if (s.empty())
        return false;
    for (char ch : s) {
        if (ch < 'a' || ch > 'z')
            return false;
    }
    return true;
}
}

AutocompleteEngineAdapter::AutocompleteEngineAdapter() = default;
AutocompleteEngineAdapter::~AutocompleteEngineAdapter() = default;

bool AutocompleteEngineAdapter::initialize(const std::string& corpusFile)
{
    engine = std::make_unique<Autocomplete>();

    if (!engine->trainFromFile(corpusFile)) {
        engine.reset();
        return false;
    }
    return true;
}

void AutocompleteEngineAdapter::processInput(char ch)
{
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (ch >= 'a' && ch <= 'z')
        prefix += ch;
}

void AutocompleteEngineAdapter::moveBack()
{
    if (!prefix.empty())
        prefix.pop_back();
}

void AutocompleteEngineAdapter::resetToRoot()
{
    prefix.clear();
}

void AutocompleteEngineAdapter::setContext(const std::string& textBeforeCursor)
{
    context = textBeforeCursor;
}

std::vector<std::string> AutocompleteEngineAdapter::getSuggestions()
{
    std::vector<std::string> out;
    if (!engine)
        return out;

    const std::string& input = context.empty() ? prefix : context;
    if (input.empty())
        return out;

    // Trigram -> bigram -> unigram backoff happens inside the model.
    for (const auto& s : engine->getSuggestions(input, 3))
        out.push_back(s.first);
    return out;
}

// A word was completed (typed + space/punctuation, or picked from the popup).
// The context set by the editor ends with the letters of that word (or with a
// space when it was picked in next-word mode).
void AutocompleteEngineAdapter::updateFrequency(const std::string& word)
{
    if (!engine)
        return;

    const std::string w = lowerAscii(word);
    if (!allLetters(w))
        return;

    std::string before = context;

    // Drop the partly typed word at the end of the context.
    while (!before.empty() &&
           std::isalpha(static_cast<unsigned char>(before.back())))
        before.pop_back();

    // The model treats a trailing space as "the last word is complete".
    engine->learnFromCompletedInput(before + w + " ");
}

// The user added a word to the dictionary: make it completable too.
void AutocompleteEngineAdapter::addWord(const std::string& word)
{
    if (!engine)
        return;

    const std::string w = lowerAscii(word);
    if (!allLetters(w))
        return;

    engine->learnFromCompletedInput(w + " ");
}
