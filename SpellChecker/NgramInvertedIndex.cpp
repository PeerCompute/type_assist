#include <iostream>
using namespace std;
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <NgramInvertedIndex.h>

#define N 3

//unordered_map<string, vector<string>> index;

void initNgram() {
    buildNgramInvertedIndex();
}
