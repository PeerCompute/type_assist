#include "wordTable.h"

int WordTable::addWord(const string& word)
{
    auto it = wordToId.find(word);

    if (it != wordToId.end())
        return it->second;

    int id = static_cast<int>(idToWord.size()) + 1;

    wordToId[word] = id;
    idToWord.push_back(word);

    return id;
}

int WordTable::getId(const string& word) const
{
    auto it = wordToId.find(word);

    if (it == wordToId.end())
        return -1;

    return it->second;
}

string WordTable::getWord(int id) const
{
    if (id <= 0 || id > static_cast<int>(idToWord.size()))
        return "";

    return idToWord[id - 1];
}

bool WordTable::contains(const string& word) const
{
    return wordToId.find(word) != wordToId.end();
}

int WordTable::size() const
{
    return static_cast<int>(idToWord.size());
}