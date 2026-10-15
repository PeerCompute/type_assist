#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;

#define ALPHABET_SIZE 26
#define MAX_DISTANCE 2

struct TrieNode {
    TrieNode* children[ALPHABET_SIZE];
    bool isEndOfWord;

    TrieNode() {
        isEndOfWord = false;

        for (int i = 0; i < ALPHABET_SIZE; i++) {
            children[i] = nullptr;
        }
    }
};

class Trie {
private:
    TrieNode* root;

    void searchRecursive(
        TrieNode* node,
        const string& target,
        const vector<int>& previousRow,
        string& currentWord,
        int maxDistance,
        vector<string>& candidates
    ) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {

            if (node->children[i] == nullptr) {
                continue;
            }

            char currentChar = 'a' + i;

            vector<int> currentRow(target.size() + 1);

            currentRow[0] = previousRow[0] + 1;

            for (int j = 1; j <= target.size(); j++) {

                int insertion = currentRow[j - 1] + 1;
                int deletion = previousRow[j] + 1;

                int substitution;

                if (target[j - 1] == currentChar) {
                    substitution = previousRow[j - 1];
                } else {
                    substitution = previousRow[j - 1] + 1;
                }

                currentRow[j] = min({
                    insertion,
                    deletion,
                    substitution
                });
            }

            int minimumDistance = *min_element(
                currentRow.begin(),
                currentRow.end()
            );

            if (minimumDistance > maxDistance) {
                continue;
            }

            currentWord.push_back(currentChar);

            if (node->children[i]->isEndOfWord &&
                currentRow[target.size()] <= maxDistance) {

                candidates.push_back(currentWord);
            }

            searchRecursive(
                node->children[i],
                target,
                currentRow,
                currentWord,
                maxDistance,
                candidates
            );

            currentWord.pop_back();
        }
    }

public:

    Trie() {
        root = new TrieNode();
    }

    void insert(const string& word) {

        TrieNode* current = root;

        for (char c : word) {

            if (c < 'a' || c > 'z') {
                continue;
            }

            int index = c - 'a';

            if (current->children[index] == nullptr) {
                current->children[index] = new TrieNode();
            }

            current = current->children[index];
        }

        current->isEndOfWord = true;
    }

    vector<string> findCandidates(
        const string& target,
        int maxDistance
    ) {

        vector<string> candidates;

        vector<int> initialRow(target.size() + 1);

        for (int i = 0; i <= target.size(); i++) {
            initialRow[i] = i;
        }

        string currentWord = "";

        searchRecursive(
            root,
            target,
            initialRow,
            currentWord,
            maxDistance,
            candidates
        );

        return candidates;
    }
};

int main() {

    Trie trie;

    ifstream file("BNC_COCA.txt");

    if (!file) {
        cout << "Could not open .txt" << endl;
        return 1;
    }

    string word;

    auto start = chrono::high_resolution_clock::now();

    while (getline(file, word)) {

        if (!word.empty()) {
            trie.insert(word);
        }
    }

    file.close();

    auto end = chrono::high_resolution_clock::now();

    auto buildTime =
        chrono::duration_cast<chrono::milliseconds>(
            end - start
        ).count();

    cout << "Time taken to build the Trie: "
         << buildTime << " ms" << endl;

    cout << "Trie built successfully." << endl;

    string input;

    cout << "Enter an incorrect word: ";
    cin >> input;

    vector<string> candidates =
        trie.findCandidates(input, MAX_DISTANCE);

    cout << "Suggestions:" << endl;

    for (const string& candidate : candidates) {
        cout << candidate << endl;
    }

    return 0;
}