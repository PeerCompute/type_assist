#ifndef WORDTABLE_H
#define WORDTABLE_H

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class WordTable
{
private:
    unordered_map<string, int> wordToId;
    vector<string> idToWord;

public:
    int addWord(const string& word);
    int getId(const string& word) const;
    string getWord(int id) const;
    bool contains(const string& word) const;
    int size() const;
};

#endif