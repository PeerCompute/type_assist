#ifndef FREQUENCY_MAP_H
#define FREQUENCY_MAP_H

#include <unordered_map>

class FrequencyMap {
private:
    std::unordered_map<int, int> frequency;

public:
    void addWord(int wordId);
    int getFrequency(int wordId) const;
    void setFrequency(int wordId, int value);
};

#endif
