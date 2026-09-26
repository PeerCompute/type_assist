#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

using namespace std;

struct XY{
    float x;
    float y;
};

struct node{
    float score;
    string word;
};

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

int main(){                 //high score == not a typo
    string str = "bat";
    minheap topk;
    vector<string> list = {"cat","rat","fat", "hat", "lat", "mat"};
    wraper(str, list, topk);

    int K = 4; // Number of top elements to extract

    for(int i = 0; i < K && !topk.tree.empty(); i++) {
        cout << topk.extractMin().word << "\n";
    }

    return 0;
}