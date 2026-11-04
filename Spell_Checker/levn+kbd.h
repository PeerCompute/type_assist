#pragma once

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define K 3
#define MAX_WORD_LEN 25

constexpr float upBnd   = 0.4f;
constexpr float lowBnd  = 0.1f;
constexpr float damper  = 0.05f;
constexpr float boost   = 0.11f;
constexpr float init    = 0.235f;

struct XY {
    float x;
    float y;
};

struct Node {
    float score;
    string word;
    float wLev;
    float wKbd;
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

static inline float charKbDist(char a, char b) {
    if (a == b) return 0.0f;
    a = static_cast<char>(tolower(a));
    b = static_cast<char>(tolower(b));
    if (a < 'a' || a > 'z' || b < 'a' || b > 'z') return 1.0f;

    const XY& p1 = KB_MAP[a - 'a'];
    const XY& p2 = KB_MAP[b - 'a'];

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dist = Sqrt(dx * dx + dy * dy);

    // Distances >= 4.0 physical units receive the maximum penalty (1.0)
    float norm = dist / 4.0f;
    return (norm > 1.0f) ? 1.0f : norm;
}

// 1D Stack-Bounded Levenshtein Distance: O(L1 * L2) time
float levenshtein(const string& s1, const string& s2) {
    int m = static_cast<int>(s1.length());
    int n = static_cast<int>(s2.length());

    if (m > MAX_WORD_LEN || n > MAX_WORD_LEN) return static_cast<float>(max(m, n));

    int dp[MAX_WORD_LEN + 1];
    for (int j = 0; j <= n; ++j) dp[j] = j;

    for (int i = 1; i <= m; ++i) {
        int prevDiag = dp[0];
        dp[0] = i;

        for (int j = 1; j <= n; ++j) {
            int temp = dp[j];
            int cost = (tolower(s1[i - 1]) == tolower(s2[j - 1])) ? 0 : 1;

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

// Positional Keyboard Distance across aligned characters
float keyboardDist(const string& s1, const string& s2) {
    int l1 = static_cast<int>(s1.length());
    int l2 = static_cast<int>(s2.length());

    if (abs(l1 - l2) > 2) {
        return 1.0f; // Rapid length discrepancy guard
    }

    int minLen = min(l1, l2);
    int maxLen = max(l1, l2);
    if (maxLen == 0) return 0.0f;

    float score = 0.0f;
    for (int i = 0; i < minLen; ++i) {
        score += charKbDist(s1[i], s2[i]);
    }

    // Trailing unmatched characters add full penalty (1.0 each)
    score += static_cast<float>(maxLen - minLen);

    return score / static_cast<float>(maxLen);
}

////////////////////////////////////////////////////////////////////////////
void computeDynamicWeights(const string& s1, const string& s2, float& outWlev, float& outWkbd) {
    int l1 = static_cast<int>(s1.length());
    int l2 = static_cast<int>(s2.length());
    int lenDiff = abs(l1 - l2);
    int maxLen = max(l1, l2);
    int minLen = min(l1, l2);

    if (maxLen == 0) {
        outWlev = 0.5f;        //default case
        outWkbd = 0.5f;
        return;
    }

    // 1. Base keyboard weight from relative length difference
    // If words are equal in length, kbdBase starts at 0.55. As length difference grows, it drops.
    float lengthDiscrepancy = static_cast<float>(lenDiff) / static_cast<float>(maxLen);
    float kbdWeight = init * (1.0f - lengthDiscrepancy);

    // 2. Modulate for word scale: longer words have higher structural redundancy,
    // so Levenshtein should dominate more for longer terms.
    if (minLen > 5) {
        kbdWeight *= (1.0f - damper * static_cast<float>(minLen - 5));
    }

    // 3. Ergonomic Coherence Factor: Check if mismatched characters are physical neighbors
    int mismatches = 0;
    int contiguousSlips = 0;
    for (int i = 0; i < minLen; ++i) {
        if (tolower(s1[i]) != tolower(s2[i])) {
            mismatches++;
            if (charKbDist(s1[i], s2[i]) <= 0.35f) { // Immediate neighbors on QWERTY
                contiguousSlips++;
            }
        }
    }

    // Boost keyboard weight if character mismatches are verified adjacent key slips
    if (mismatches > 0 && mismatches <= 2) {
        float slipRatio = static_cast<float>(contiguousSlips) / static_cast<float>(mismatches);
        kbdWeight += (boost * slipRatio);
    }

    // 4. Boundary Clamping: Keep W_kbd in safe [lowBnd, upBnd] range
    kbdWeight = max(lowBnd, min(upBnd, kbdWeight));


    outWkbd = kbdWeight;
    outWlev = 1.0f - kbdWeight;
}

// Bounded Max-Heap to track the K lowest scores
class TopKTracker {
private:
    Node heap[K];
    int count;

    void heapifyUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (heap[i].score > heap[p].score) {
                swap(heap[i], heap[p]);
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
                swap(heap[i], heap[largest]);
                i = largest;
            } else {
                break;
            }
        }
    }

public:
    TopKTracker() : count(0) {}

    void insert(float score, const string& word, float wLev, float wKbd) {
        if (count < K) {
            heap[count] = {score, word, wLev, wKbd};
            heapifyUp(count);
            count++;
        } else if (score < heap[0].score) {
            heap[0] = {score, word, wLev, wKbd};
            heapifyDown(0);
        }
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

    int incorrectLen = static_cast<int>(incorrectWord.length());
    for (const auto& candidate : list) {
        // Step 1: Compute custom dynamic weights for this exact (query, candidate) pair
        float wLev, wKbd;
        computeDynamicWeights(incorrectWord, candidate, wLev, wKbd);

        // Step 2: Compute individual normalized metrics
        float levScore = levenshtein(incorrectWord, candidate);

        float kbScore = keyboardDist(incorrectWord, candidate);

        float maxLen = static_cast<float>(max(incorrectWord.length(), candidate.length()));
        float normLev = (maxLen == 0.0f) ? 0.0f : (levScore / maxLen);

        // Step 3: Compute final composite score from the dynamic weights
        float totalScore = (wLev * normLev) + (wKbd * kbScore);

        tracker.insert(totalScore, candidate, wLev, wKbd);
        
    }

    vector<string> listt;
    vector<Node> node;

    node = tracker.getSortedResults();
    for(size_t i = 0; i < node.size(); i++) {
        listt.push_back(node[i].word);
    }

    return listt;

    // return tracker.getSortedResults();
}

// vector<string> recommendation(const string& incorrectWord, const vector<string>& list) {

//     TopKTracker tracker;

//     for (size_t i = 0; i < list.size(); ++i) {
//         const string& candidate = list[i];

//         float levScore = levenshtein(incorrectWord, candidate);
//         float kbScore = keyboardDist(incorrectWord, candidate);


//         size_t maxLen;
//         if (incorrectWord.length() > candidate.length()) {
//             maxLen = incorrectWord.length();
//         } else {
//             maxLen = candidate.length();
//         }
//         float normLev = (maxLen == 0) ? 0.0f : (levScore / static_cast<float>(maxLen));
        
//         float totalScore = (Wlev * normLev) + (Wkbd * kbScore);

//         tracker.insert(totalScore, candidate);
//     }

//     vector<string> listt;
//     vector<Node> node;

//     node = tracker.getSortedResults();
//     for(size_t i = 0; i < node.size(); i++) {
//         listt.push_back(node[i].word);
//     }

//     return listt;
//     //return tracker.getSortedResults();
// }   
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
