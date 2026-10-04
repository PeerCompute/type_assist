#ifndef FREQUENCY_MAP_H
#define FREQUENCY_MAP_H

#include <string>
#include <unordered_map>

using namespace std;

class FrequencyMap {
private:
    unordered_map<string, int> frequency;

public:
    void addWord(const string& word);
    int getFrequency(const string& word);
};

#endif