#include "FrequencyMap.h"

void FrequencyMap::addWord(const string& word) {
    frequency[word]++;
}

int FrequencyMap::getFrequency(const string& word) {
    return frequency[word];
}