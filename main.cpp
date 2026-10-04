#include <iostream>
#include "FrequencyMap.h"

using namespace std;

int main() {
    FrequencyMap frequencyMap;

    frequencyMap.addWord("apple");
    frequencyMap.addWord("apple");
    frequencyMap.addWord("apple");

    frequencyMap.addWord("banana");
    frequencyMap.addWord("banana");

    cout << "apple: " << frequencyMap.getFrequency("apple") << endl;
    cout << "banana: " << frequencyMap.getFrequency("banana") << endl;
    cout << "orange: " << frequencyMap.getFrequency("orange") << endl;

    return 0;
}