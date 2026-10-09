#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <unordered_map>

using namespace std;

//_____________________________________________________________ngrm

#define N_VAL 3
#define TOP_K 5

struct Node {
    float score;
    string word;
};
//_____________________________________________________________kbdist

struct XY{
    float x;
    float y;
};

struct node{
    float score;
    string word;
};

//____________________________________________________________ngrm functions

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

//____________________________________________________________kbd functions

struct minheap{
    vector<node> tree;

    int parent(int i) {return (i - 1) / 2;}
    int left(int i) {return (2 * i) + 1;}
    int right(int i) {return (2 * i) + 2;}

    void heapifyUp(int i) {
        while (i > 0 && tree[parent(i)].score > tree[i].score) {
            std::swap(tree[parent(i)], tree[i]);
            i = parent(i);
        }
    }

    void heapifyDown(int i) {
        int smallest = i;
        int l = left(i);
        int r = right(i);

        if (l < tree.size() && tree[l].score < tree[smallest].score)
            smallest = l;

        if (r < tree.size() && tree[r].score < tree[smallest].score)
            smallest = r;

        if (smallest != i) {
            std::swap(tree[i], tree[smallest]);
            heapifyDown(smallest);
        }
    }

    void insert(float score, const std::string& word) { // Passed by const ref
        tree.push_back({score, word});
        heapifyUp(static_cast<int>(tree.size()) - 1);
    }

    node extractMin() {
        if (tree.empty()) return {-1.0f, ""};

        node root = tree[0];
        tree[0] = tree.back();
        tree.pop_back();
        
        if (!tree.empty()) {
            heapifyDown(0);
        }
        
        return root;
    }
};

static unordered_map<char, XY> kb_map = {
    // Row 0 (Top letter row, y = 0.0)
    {'q', {0.0f, 0.0f}}, {'w', {1.0f, 0.0f}}, {'e', {2.0f, 0.0f}},
    {'r', {3.0f, 0.0f}}, {'t', {4.0f, 0.0f}}, {'y', {5.0f, 0.0f}},
    {'u', {6.0f, 0.0f}}, {'i', {7.0f, 0.0f}}, {'o', {8.0f, 0.0f}},
    {'p', {9.0f, 0.0f}},

    // Row 1 (Home row, y = 1.0, staggered by +0.5)
    {'a', {0.5f, 1.0f}}, {'s', {1.5f, 1.0f}}, {'d', {2.5f, 1.0f}},
    {'f', {3.5f, 1.0f}}, {'g', {4.5f, 1.0f}}, {'h', {5.5f, 1.0f}},
    {'j', {6.5f, 1.0f}}, {'k', {7.5f, 1.0f}}, {'l', {8.5f, 1.0f}},

    // Row 2 (Bottom row, y = 2.0, staggered by +1.0)
    {'z', {1.0f, 2.0f}}, {'x', {2.0f, 2.0f}}, {'c', {3.0f, 2.0f}},
    {'v', {4.0f, 2.0f}}, {'b', {5.0f, 2.0f}}, {'n', {6.0f, 2.0f}},
    {'m', {7.0f, 2.0f}}
};

float kebrdDist(char a, char b){        //a, b lower case assume
    if(a == b) return 0.0;
    float val = 0.0;

    //euclidean dist
    float x1 = kb_map[a].x, y1 = kb_map[a].y;
    float x2 = kb_map[b].x, y2 = kb_map[b].y;
/*
    val = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));          //alternate 2 ways to test

    //manhattan dist
    val = fabs(x2-x1) + fabs(y2-y1);
*/
    //normalized
    float max = 3.5f;
    val = min(1.0f, sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)) / max);

    return  val;
}

float wrddist(const string& s1, const string& s2){
    if(s1.length() != s2.length()) return 1.0f;
    if(s1.length() == 0) return 0.0f;

    float score = 0.0f;
    int i = 0;
    while (i < s1.length()){
        score += kebrdDist(s1[i], s2[i]);
        i++;
    }

    return score / s1.length();
}

void wraper(const string& s1, const vector<string>& list, minheap& topk){
    int i = 0;
    float score = 0.0f;
    while(i < list.size()){
        score = wrddist(s1, list[i]);
        topk.insert(score, list[i]);
        i++;
    }
}

//____________________________________________________________main

int main(){

    string userwrd = "apple";
    vector<string> list1 = {"orange", "apples", "apply", "ape", "application", "apple", "plea"};

    printTopK(userwrd, list1);

    //________________________________________________________
    cout << "\n\nkbord dist \n";
    string str = "bat";
    minheap topk;
    vector<string> list2 = {"cat","rat","fat", "hat", "lat", "mat"}; //high score == not a typo
    wraper(str, list2, topk);

    int K = 4; // Number of top elements to extract

    for(int i = 0; i < K && !topk.tree.empty(); i++) {
        cout << topk.extractMin().word << "\n";
    }

    return 0;
}