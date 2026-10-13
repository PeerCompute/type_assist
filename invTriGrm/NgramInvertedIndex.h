#ifndef NGRAM_INDEX_H
#define NGRAM_INDEX_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>

#define N 3

using namespace std;

void getNgrams(const string& word, vector<string>& ngrams){
    for(uint32_t i = 0; i <= word.length() - N; i++){
        string ngram = word.substr(i, N);
        ngrams.push_back(ngram);
    }
}

void generateNgrams(unordered_map<string, vector<uint32_t>>& index, const vector<string>& words){
    for(uint32_t i = 0; i < words.size(); i++){
        string word = words[i];

        word = "**" + word + "**";

        for(uint32_t j = 0; j <= word.length() - N; j++){
            string ngram = word.substr(j, N);
            if(index.find(ngram) != index.end()){
                index[ngram].push_back(i);
            } else {
                index[ngram] = vector<uint32_t>{i};
            }
        }
    }
}

void buildNgramInvertedIndex(unordered_map<string, vector<uint32_t>>* index, vector<string>& words){
    auto start = std::chrono::high_resolution_clock::now();

    //read from a dictionary file
    ifstream dictFile("../dictionary-100k.txt");

    //sequence to follow
    //extract each word from the dictionary file
    //for each word, generate n-grams
    //check if the ngram exists in the index, if it does, add the word to the vector of words for that ngram
    //else add new entry in the index with the ngram as key and the word as value in a vector
    for(string word; dictFile >> word;){
        words.push_back(word);
    }
/*
    for(int i = 0; i < 160; i++){  
        index->clear();

        dictFile.clear();  // clear EOF flag
        dictFile.seekg(0); // go back to beginning

        generateNgrams(*index, words);
    }*/
    generateNgrams(*index, words);
    dictFile.close();

    // ofstream outFile("medicalNgram.txt");

    // for(auto entry : *index){
    //     outFile << entry.first << " : ";
    //     for(auto word : entry.second){
    //         outFile << word << " ";
    //     }
    //     outFile << endl;
    // }
    // outFile.close();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken to build the n-gram inverted index: " << duration.count() << " ms" << std::endl;
}

#endif
