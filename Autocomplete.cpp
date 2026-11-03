#include "Autocomplete.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;


// ------------------------------------
// NORMALIZE WORD
// ------------------------------------

static string normalizeWord(const string& word)
{
    string result;

    for (unsigned char ch : word)
    {
        if (isalpha(ch))
        {
            result += static_cast<char>(tolower(ch));
        }
    }

    return result;
}


// ------------------------------------
// TOKENIZE INPUT
// ------------------------------------

vector<string> Autocomplete::tokenize(const string& text) const
{
    vector<string> words;

    stringstream ss(text);

    string word;

    while (ss >> word)
    {
        word = normalizeWord(word);

        if (!word.empty())
            words.push_back(word);
    }

    return words;
}


// ------------------------------------
// TRAIN FROM CORPUS
// ------------------------------------

bool Autocomplete::trainFromFile(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
        return false;

    string line;

    bool trained = false;

    while (getline(file, line))
    {
        vector<string> words = tokenize(line);

        if (!words.empty())
        {
            ngramTrie.addSentence(words, wordTable);

            trained = true;
        }
    }

    file.close();

    if (trained)
    {
        ngramTrie.buildTopK(wordTable);
    }

    return trained;
}


// ------------------------------------
// GET SUGGESTIONS
// ------------------------------------

vector<pair<string, int>> Autocomplete::getSuggestions(
    const string& input,
    int k
) const
{
    vector<string> words = tokenize(input);

    vector<string> previousWords;

    string prefix = "";

    bool trailingSpace =
        !input.empty() &&
        isspace(static_cast<unsigned char>(input.back()));

    if (trailingSpace)
    {
        previousWords = words;
    }
    else if (!words.empty())
    {
        prefix = words.back();

        words.pop_back();

        previousWords = words;
    }

    return ngramTrie.predict(
        previousWords,
        prefix,
        wordTable,
        k
    );
}


// ------------------------------------
// LEARN FROM COMPLETED WORD
// ------------------------------------

void Autocomplete::learnFromCompletedInput(
    const string& input
)
{
    if (input.empty())
        return;

    if (!isspace(static_cast<unsigned char>(input.back())))
        return;

    vector<string> words = tokenize(input);

    if (words.empty())
        return;

    // Last word is the completed word.
    string completedWord = words.back();

    words.pop_back();

    // Update user frequency.
    ngramTrie.observeWord(
        words,
        completedWord,
        wordTable
    );
}