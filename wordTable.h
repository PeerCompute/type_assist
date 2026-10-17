#ifndef WORD_TABLE_H
#define WORD_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>

class WordTable {
private:
    std::unordered_map<std::string, int> wordToId;
    std::vector<std::string> idToWord;

public:
    int getOrCreateId(const std::string& word);
    int getId(const std::string& word) const;
    std::string getWord(int id) const;
    const std::vector<std::string>& getAllWords() const;
};

#endif
