#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>

using namespace std;

#define N_VAL 3
#define TOP_K 5

struct Node {
    float score;
    string word;
};

class MinHeap {
private:
    Node heap[TOP_K];
    int size;

    void swapNodes(int i, int j) {
        Node temp = heap[i];
        heap[i] = heap[j];
        heap[j] = temp;
    }

    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (heap[index].score < heap[parent].score) {
                swapNodes(index, parent);
                index = parent;
            } else {
                break;
            }
        }
    }

    void heapifyDown(int index) {
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;

            if (left < size && heap[left].score < heap[smallest].score) smallest = left;
            if (right < size && heap[right].score < heap[smallest].score) smallest = right;

            if (smallest != index) {
                swapNodes(index, smallest);
                index = smallest;
            } else {
                break;
            }
        }
    }

public:
    MinHeap() : size(0) {}

    int getSize() const {
        return size;
    }

    void insert(float score, const string& word) {
        if (size < TOP_K) {
            // Heap is not full: append and bubble up
            heap[size] = {score, word};
            heapifyUp(size);
            size++;
        } else if (score > heap[0].score) {
            // New score exceeds current minimum in top K: replace root and bubble down
            heap[0] = {score, word};
            heapifyDown(0);
        }
    }

    Node pop() {
        Node root = heap[0];
        heap[0] = heap[size - 1];
        size--;
        heapifyDown(0);
        return root;
    }
};

float wrdCmp(const string& s1, const string& s2) {
    if (s1.empty() || s2.empty()) return 0.0f;
    if (s1 == s2) return 1.0f;

    float score;
    unordered_set<string> set1;

    for (size_t i = 0; i + N_VAL <= s1.size(); i++) {   //ngram str1
        set1.insert(s1.substr(i, N_VAL));
    }

    unordered_set<string> set2;
    for (size_t i = 0; i + N_VAL <= s2.size(); i++) {   //ngram str2
        set2.insert(s2.substr(i, N_VAL));
    }

    float intersectionCount = 0.0f;
    for (const string& gram : set1) {
        if (set2.find(gram) != set2.end()) {
            intersectionCount++;
        }
    }

    float unionCount = static_cast<float>(set1.size() + set2.size()) - intersectionCount;
    if (unionCount == 0.0f) return 0.0f;

    // // Sørensen-Dice: 2 * |A ∩ B| / (|A| + |B|)
    // score = (2.0f * intersectionCount) / static_cast<float>(set1.size() + set2.size());

    // //Overlap Coefficient
    // score = intersectionCount / min(set1.size(), set2.size());

    // Jaccard similarity: |A ∩ B| / |A ∪ B|
    score = intersectionCount / unionCount;
    return score;
}

void printTopK(const string& user, const vector<string>& list) {
    string paddedUser = "";
    for (int i = 0; i < N_VAL - 1; i++) paddedUser.push_back('*');          //padding user word
    paddedUser += user;
    for (int i = 0; i < N_VAL - 1; i++) paddedUser.push_back('*');

    MinHeap heap;
    for (size_t i = 0; i < list.size(); i++) {
        string paddedWord = "";
        for (int j = 0; j < N_VAL - 1; j++) paddedWord.push_back('*');      //padding list words
        paddedWord += list[i];
        for (int j = 0; j < N_VAL - 1; j++) paddedWord.push_back('*');

        float score = wrdCmp(paddedUser, paddedWord);
        heap.insert(score, list[i]);                    //insert in heap
    }

    // 3. Extract and display in descending order (highest score first)
    int count = heap.getSize();
    Node results[TOP_K];

    // Pop extracts in ascending order; store from the end to reverse
    for (int i = count - 1; i >= 0; i--) {
        results[i] = heap.pop();
    }

    for (int i = 0; i < count; i++) {
        cout << results[i].word << " : " << results[i].score << "\n";
    }
}

int main() {
    string userwrd = "apple";
    vector<string> list = {"orange", "apples", "apply", "ape", "application", "apple", "plea"};

    printTopK(userwrd, list);

    return 0;
}