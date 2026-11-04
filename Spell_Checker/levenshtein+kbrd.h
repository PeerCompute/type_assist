#pragma once

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define K 5
#define MAX_WORD_LEN 20
#define Wkbd 0.5f
#define Wlev 0.5f

struct XY {
    float x;
    float y;
};

struct Node {
    float score;
    string word;
};

static const XY KB_MAP[26] = {
    {0.5f, 1.0f}, // a
    {5.0f, 2.0f}, // b
    {3.0f, 2.0f}, // c
    {2.5f, 1.0f}, // d
    {2.0f, 0.0f}, // e
    {3.5f, 1.0f}, // f
    {4.5f, 1.0f}, // g
    {5.5f, 1.0f}, // h
    {7.0f, 0.0f}, // i
    {6.5f, 1.0f}, // j
    {7.5f, 1.0f}, // k
    {8.5f, 1.0f}, // l
    {7.0f, 2.0f}, // m
    {6.0f, 2.0f}, // n
    {8.0f, 0.0f}, // o
    {9.0f, 0.0f}, // p
    {0.0f, 0.0f}, // q
    {3.0f, 0.0f}, // r
    {1.5f, 1.0f}, // s
    {4.0f, 0.0f}, // t
    {6.0f, 0.0f}, // u
    {4.0f, 2.0f}, // v
    {1.0f, 0.0f}, // w
    {2.0f, 2.0f}, // x
    {5.0f, 0.0f}, // y
    {1.0f, 2.0f}  // z
};

static inline float Sqrt(float n) {
    if (n <= 0.0f) return 0.0f;
    float root = n;
    for (int i = 0; i < 4; ++i) {
        root = 0.5f * (root + n / root);
    }
    return root;
}

// Single-char physical keyboard distance
static inline float charKbDist(char a, char b) {
    if (a == b) return 0.0f;
    if (a < 'a' || a > 'z' || b < 'a' || b > 'z') return 1.0f;

    const XY& p1 = KB_MAP[a - 'a'];
    const XY& p2 = KB_MAP[b - 'a'];

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dist = Sqrt(dx * dx + dy * dy);

    float norm = dist / 3.5f;
    return (norm > 1.0f) ? 1.0f : norm;
}

float levenshtein(const string& s1, const string& s2) {
    int m = s1.length();
    int n = s2.length();

    if (m > MAX_WORD_LEN || n > MAX_WORD_LEN) return 100.0f;

    int dp[MAX_WORD_LEN + 1];
    for (int j = 0; j <= n; ++j) dp[j] = j;

    for (int i = 1; i <= m; ++i) {
        int prevDiag = dp[0];
        dp[0] = i;

        for (int j = 1; j <= n; ++j) {
            int temp = dp[j];
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;

            int ins = dp[j - 1] + 1;
            int del = dp[j] + 1;
            int rep = prevDiag + cost;

            int minVal = (ins < del) ? ins : del;
            if (rep < minVal) minVal = rep;

            dp[j] = minVal;
            prevDiag = temp;
        }
    }
    return static_cast<float>(dp[n]);
}

float keyboardDist(const string& s1, const string& s2) {
    int l1 = s1.length();
    int l2 = s2.length();
    if(l1-l2 > 2 || l1-l2 < -2){
        return 1.0f;
    }
    int minLen = (l1 < l2) ? l1 : l2;
    int maxLen = (l1 > l2) ? l1 : l2;

    if (maxLen == 0) return 0.0f;

    float score = 0.0f;
    for (int i = 0; i < minLen; ++i) {
        score += charKbDist(s1[i], s2[i]);
    }

    // Unmatched trailing characters add maximum physical cost
    score += static_cast<float>(maxLen - minLen);

    return score / static_cast<float>(maxLen);
}

// Bounded Max-Heap to track K lowest scores
class TopKTracker {
private:
    Node heap[K];
    int count;

    void heapifyUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (heap[i].score > heap[p].score) {
                Node tmp = heap[i];
                heap[i] = heap[p];
                heap[p] = tmp;
                i = p;
            } else {
                break;
            }
        }
    }

    void heapifyDown(int i) {
        while (true) {
            int l = 2 * i + 1;
            int r = 2 * i + 2;
            int largest = i;

            if (l < count && heap[l].score > heap[largest].score) largest = l;
            if (r < count && heap[r].score > heap[largest].score) largest = r;

            if (largest != i) {
                Node tmp = heap[i];
                heap[i] = heap[largest];
                heap[largest] = tmp;
                i = largest;
            } else {
                break;
            }
        }
    }

public:
    TopKTracker() : count(0) {}

    void insert(float score, const string& word) {
        if (count < K) {
            heap[count] = {score, word};
            heapifyUp(count);
            count++;
        } else if (score < heap[0].score) {
            heap[0] = {score, word};
            heapifyDown(0);
        }
    }

    int size(){ 
        return count;
    }

    vector<Node> getSortedResults() {
        vector<Node> res(count);
        int total = count;
        for (int i = total - 1; i >= 0; --i) {
            res[i] = heap[0];
            heap[0] = heap[count - 1];
            count--;
            heapifyDown(0);
        }
        return res;
    }
};


vector<string> recommendation(const string& incorrectWord, const vector<string>& list) {

    TopKTracker tracker;

    for (size_t i = 0; i < list.size(); ++i) {
        const string& candidate = list[i];

        float levScore = levenshtein(incorrectWord, candidate);
        float kbScore = keyboardDist(incorrectWord, candidate);


        size_t maxLen;
        if (incorrectWord.length() > candidate.length()) {
            maxLen = incorrectWord.length();
        } else {
            maxLen = candidate.length();
        }
        float normLev = (maxLen == 0) ? 0.0f : (levScore / static_cast<float>(maxLen));
        
        float totalScore = (Wlev * normLev) + (Wkbd * kbScore);

        tracker.insert(totalScore, candidate);
    }

    vector<string> listt;
    vector<Node> node;

    node = tracker.getSortedResults();
    for(size_t i = 0; i < node.size(); i++) {
        listt.push_back(node[i].word);
    }

    return listt;
    //return tracker.getSortedResults();
}   
/*
int main() {
    string incWrd = "playng"; // Example incorrect word
    vector<string> list = {
    "cat", "car", "can", "cap", "cab", "cake", "call", "calm", "came", "camp", "card",
    "care", "case", "cash", "cast", "cave", "cell", "cent", "chat", "chef", "cheek",
    "cheer", "cheese", "cheque", "chess", "chest", "chief", "child", "chill", "china",
    "choice", "choose", "church", "circle", "circuit", "citizen", "classic", "climate",
    "clock", "closet", "coffee", "collar", "college", "colour", "comfort", "command",
    "computer", "complete", "consider", "construction", "the", "and", "cat", "bat", 
    "rat", "hat", "fat", "mat", "pat", "car", "can", "cut", "dog", "fog", "log", "run",
    "sun", "fun", "red", "bed", "pen", "pin",

    // Common 4-6 letter words
    "book", "back", "bark", "dark", "park", "word", "work", "walk", "wall",
    "ball", "call", "fall", "tall", "hand", "band", "land", "sand", "home",
    "come", "some", "time", "line", "life", "live", "like", "look", "took",
    "food", "good", "wood", "mood", "fast", "last", "past", "cast", "cost",
    "post", "most", "host", "help", "held", "hold", "gold", "cold", "told",

    // Medium-length words (common conversational vocabulary)
    "friend", "family", "school", "street", "window", "yellow", "orange",
    "purple", "market", "garden", "player", "action", "nation", "person",
    "people", "system", "method", "vector", "string", "simple", "sample",
    "middle", "bottle", "little", "settle", "handle", "candle", "bundle",
    "butter", "better", "bitter", "letter", "winter", "summer", "spring",

    // Words with common suffixes and prefixes (-ing, -tion, -ment, pre-, dis-)
    "testing", "running", "walking", "talking", "reading", "writing", "playing",
    "feeling", "looking", "working", "holding", "helping", "opening", "closing",
    "movement", "judgment", "payment", "shipment", "agreement", "treatment",
    "station", "section", "mention", "fraction", "reaction", "position",
    "condition", "relation", "solution", "decision", "provision", "revision",

    // Longer academic & professional vocabulary
    "important", "different", "difficult", "necessary", "president", "government",
    "development", "environment", "information", "organization", "communication",
    "transportation", "understanding", "programming", "performance", "algorithm",
    "similarity", "coefficient", "candidate", "correction", "keyboard", "distance"};

    vector<Node> topk = recomendation(incWrd, list);

    cout << "Top " << topk.size() << " recommendations for \"" << incWrd << "\":\n";
    for (size_t i = 0; i < topk.size(); ++i) {
        cout << i + 1 << ". " << topk[i].word << " ( Score: " << topk[i].score << ")\n";
    }

    return 0;
}*/
