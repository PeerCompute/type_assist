#ifndef WORD_TABLE_H
#define WORD_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// Maps words <-> integer ids. Ids start at 1 (0 is never used).
class WordTable
{
private:
    unordered_map<string, int> wordToId;
    vector<string> idToWord;

public:
    int addWord(const string& word);          // returns existing id or creates one
    int getId(const string& word) const;      // -1 if the word is unknown
    string getWord(int id) const;             // "" if the id is invalid
    bool contains(const string& word) const;
    int size() const;
};

#endif
