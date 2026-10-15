#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;

#define ALPHABET_SIZE 26
#define START_DISTANCE 2

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
    long long nodeCount;

    void searchRecursive(
        TrieNode* node,
        const string& target,
        const vector<int>& previousRow,
        string& currentWord,
        int maxDistance,
        vector<string>& candidates
    ) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {

            if (node->children[i] == nullptr)
                continue;

            char currentChar = 'a' + i;

            vector<int> currentRow(target.size() + 1);

            currentRow[0] = previousRow[0] + 1;

            for (int j = 1; j <= target.size(); j++) {

                int insertion = currentRow[j - 1] + 1;

                int deletion = previousRow[j] + 1;

                int substitution;

                if (target[j - 1] == currentChar)
                    substitution = previousRow[j - 1];
                else
                    substitution = previousRow[j - 1] + 1;

                currentRow[j] =
                    min({insertion, deletion, substitution});
            }

            int minimumDistance =
                *min_element(currentRow.begin(), currentRow.end());

            if (minimumDistance > maxDistance)
                continue;

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
        nodeCount = 1;
    }

    void insert(const string& word) {

        TrieNode* current = root;

        for (char c : word) {

            if (c < 'a' || c > 'z')
                continue;

            int index = c - 'a';

            if (current->children[index] == nullptr) {

                current->children[index] = new TrieNode();
                nodeCount++;
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

        for (int i = 0; i <= target.size(); i++)
            initialRow[i] = i;

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

    long long getNodeCount() {
        return nodeCount;
    }

    long long getTrieMemory() {
        return nodeCount * sizeof(TrieNode);
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

    // Trie building time
    auto buildStart = chrono::high_resolution_clock::now();

    while (getline(file, word)) {

        if (!word.empty())
            trie.insert(word);
    }

    file.close();

    auto buildEnd = chrono::high_resolution_clock::now();

    auto buildTime =
        chrono::duration_cast<chrono::milliseconds>(
            buildEnd - buildStart
        ).count();

    // Trie space
    long long trieMemory = trie.getTrieMemory();

    double trieKB = trieMemory / 1024.0;
    double trieMB = trieMemory / (1024.0 * 1024.0);

    cout << "\nTrie Information" << endl;
    cout << "--------------------------" << endl;

    cout << "Number of Trie nodes: "
         << trie.getNodeCount() << endl;

    cout << "Trie building time: "
         << buildTime << " ms" << endl;

    cout << "Trie space: "
         << trieMemory << " bytes" << endl;

    cout << "Trie space: "
         << trieKB << " KB" << endl;

    cout << "Trie space: "
         << trieMB << " MB" << endl;

    // Input
    string input;

    cout << "\nEnter an incorrect word: ";
    cin >> input;

    // Candidate generation
    auto searchStart =
        chrono::high_resolution_clock::now();

    int maxDistance = START_DISTANCE;

    vector<string> candidates;

    while (candidates.empty()) {

        candidates =
            trie.findCandidates(input, maxDistance);

        if (!candidates.empty())
            break;

        maxDistance++;

        if (maxDistance > 10)
            break;
    }

    auto searchEnd =
        chrono::high_resolution_clock::now();

    auto searchTime =
        chrono::duration_cast<chrono::milliseconds>(
            searchEnd - searchStart
        ).count();

    // Approximate candidate generation space
    long long dpSpace =
        2LL * (input.length() + 1) * sizeof(int);

    long long currentWordSpace =
        (input.length() + 1) * sizeof(char);

    long long candidateSpace = 0;

    for (const string& candidate : candidates) {

        candidateSpace +=
            candidate.capacity() * sizeof(char);
    }

    long long candidateGenerationSpace =
        dpSpace +
        currentWordSpace +
        candidateSpace;

    double candidateKB =
        candidateGenerationSpace / 1024.0;

    double candidateMB =
        candidateGenerationSpace / (1024.0 * 1024.0);

    cout << "\nCandidate Generation" << endl;
    cout << "--------------------------" << endl;

    cout << "Maximum edit distance used: "
         << maxDistance << endl;

    cout << "Number of candidates: "
         << candidates.size() << endl;

    cout << "Candidate generation time: "
         << searchTime << " ms" << endl;

    cout << "Approximate candidate generation space: "
         << candidateGenerationSpace << " bytes" << endl;

    cout << "Approximate candidate generation space: "
         << candidateKB << " KB" << endl;

    cout << "Approximate candidate generation space: "
         << candidateMB << " MB" << endl;

    cout << "\nSuggestions:" << endl;

    for (const string& candidate : candidates)
        cout << candidate << endl;

    return 0;
}
