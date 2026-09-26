#include <iostream>
using namespace std;
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>

#define N 3


void buildNgramInvertedIndex(){}

void generateNgrams(unordered_map<string, vector<string>>& index, ifstream& dictFile){
    for(string word; dictFile >> word;){
        for(int i = 0; i <= word.length() - N; i++){
            string ngram = word.substr(i, N);
            if(index.find(ngram) != index.end()){
                index[ngram].push_back(word);
            } else {
                index[ngram] = vector<string>{word};
            }
        }
    }
}

int main(){
    unordered_map<string, vector<string>> index;
    
    //read from a dictionary file
    ifstream dictFile("dictionary.txt");

    //sequence to follow
    //extract each word from the dictionary file
    //for each word, generate n-grams
    //check if the ngram exists in the index, if it does, add the word to the vector of words for that ngram
    //else add new entry in the index with the ngram as key and the word as value in a vector
    generateNgrams(index, dictFile);
    dictFile.close();

    for(auto entry : index){
        cout << entry.first << " : ";
        for(auto word : entry.second){
            cout << word << " ";
        }
        cout << endl;
    }
}