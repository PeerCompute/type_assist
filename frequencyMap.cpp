#include "frequencyMap.h"

void FrequencyMap::addWord(int wordId) {
    ++frequency[wordId];
}

int FrequencyMap::getFrequency(int wordId) const {
    const auto it = frequency.find(wordId);
    return it == frequency.end() ? 0 : it->second;
}

void FrequencyMap::setFrequency(int wordId, int value) {
    if (wordId > 0 && value >= 0) {
        frequency[wordId] = value;
    }
}
