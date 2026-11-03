#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cctype>
#include <set>
#include <map>

using namespace std;

#define K 3
#define MAX_WORD_LEN 32

// Dynamic Adaptive Weight Hyperparameters
constexpr float upBnd   = 0.6f;
constexpr float lowBnd  = 0.1f;
constexpr float damper  = 0.035f;
constexpr float boost   = 0.08f;
constexpr float init    = 0.5f;

const string TEST_CASES_CSV = "new_test_cases_600.csv";         //input test cases
const string LEXICON_FILE    = "lexicon_600.txt";
const string ANALYTICS_FILE  = "hybrid_analytics.txt";             //output analytics file

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

struct TestCase {
    int id;
    string query;
    string groundTruth;
    string category;
    string type;
    int truthLen;
    string notes;
};

struct QueryResult {
    TestCase tc;
    int rank;
    float bestScore;
    float wLev;
    float wKbd;
    double latencyUs;
    bool inLexicon;
    bool passedTop1;
    bool passedTop5;
};

// 2D Staggered QWERTY coordinates
static const XY KB_MAP[26] = {
    {0.8f, 1.0f}, // a
    {5.2f, 2.0f}, // b
    {3.2f, 2.0f}, // c
    {2.8f, 1.0f}, // d
    {2.5f, 0.0f}, // e
    {3.8f, 1.0f}, // f
    {4.8f, 1.0f}, // g
    {5.8f, 1.0f}, // h
    {7.5f, 0.0f}, // i
    {6.8f, 1.0f}, // j
    {7.8f, 1.0f}, // k
    {8.8f, 1.0f}, // l
    {7.2f, 2.0f}, // m
    {6.2f, 2.0f}, // n
    {8.5f, 0.0f}, // o
    {9.5f, 0.0f}, // p
    {0.5f, 0.0f}, // q
    {3.5f, 0.0f}, // r
    {1.8f, 1.0f}, // s
    {4.5f, 0.0f}, // t
    {6.5f, 0.0f}, // u
    {4.2f, 2.0f}, // v
    {1.5f, 0.0f}, // w
    {2.2f, 2.0f}, // x
    {5.5f, 0.0f}, // y
    {1.2f, 2.0f}  // z
};

static inline float charKbDist(char a, char b) {
    if (a == b) return 0.0f;
    a = static_cast<char>(tolower(static_cast<unsigned char>(a)));
    b = static_cast<char>(tolower(static_cast<unsigned char>(b)));
    if (a < 'a' || a > 'z' || b < 'a' || b > 'z') return 1.0f;

    const XY& p1 = KB_MAP[a - 'a'];
    const XY& p2 = KB_MAP[b - 'a'];

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dist = sqrt(dx * dx + dy * dy);

    float norm = dist / 4.0f;
    return (norm > 1.0f) ? 1.0f : norm;
}

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
            int cost = (tolower(static_cast<unsigned char>(s1[i - 1])) == 
                        tolower(static_cast<unsigned char>(s2[j - 1]))) ? 0 : 1;

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
    int l1 = static_cast<int>(s1.length());
    int l2 = static_cast<int>(s2.length());

    if (abs(l1 - l2) > 2) {
        return 1.0f;
    }

    int minLen = min(l1, l2);
    int maxLen = max(l1, l2);
    if (maxLen == 0) return 0.0f;

    float score = 0.0f;
    for (int i = 0; i < minLen; ++i) {
        score += charKbDist(s1[i], s2[i]);
    }

    score += static_cast<float>(maxLen - minLen);
    return score / static_cast<float>(maxLen);
}

void computeDynamicWeights(const string& s1, const string& s2, float& outWlev, float& outWkbd) {
    int l1 = static_cast<int>(s1.length());
    int l2 = static_cast<int>(s2.length());
    int lenDiff = abs(l1 - l2);
    int maxLen = max(l1, l2);
    int minLen = min(l1, l2);

    if (maxLen == 0) {
        outWlev = 0.50f;
        outWkbd = 0.50f;
        return;
    }

    // 1. Base keyboard weight from relative length difference
    float lengthDiscrepancy = static_cast<float>(lenDiff) / static_cast<float>(maxLen);
    float kbdWeight = init * (1.0f - lengthDiscrepancy);

    // 2. Modulate for word scale
    if (minLen > 5) {
        kbdWeight *= (1.0f - damper * static_cast<float>(minLen - 5));
    }

    // 3. Ergonomic Coherence Factor
    int mismatches = 0;
    int contiguousSlips = 0;
    for (int i = 0; i < minLen; ++i) {
        if (tolower(static_cast<unsigned char>(s1[i])) != tolower(static_cast<unsigned char>(s2[i]))) {
            mismatches++;
            if (charKbDist(s1[i], s2[i]) <= 0.35f) {
                contiguousSlips++;
            }
        }
    }

    if (mismatches > 0 && mismatches <= 2) {
        float slipRatio = static_cast<float>(contiguousSlips) / static_cast<float>(mismatches);
        kbdWeight += (boost * slipRatio);
    }

    // 4. Clamping bounds
    kbdWeight = max(lowBnd, min(upBnd, kbdWeight));

    outWkbd = kbdWeight;
    outWlev = 1.0f - kbdWeight;
}

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
            } else break;
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
            } else break;
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

vector<Node> recomendation(const string& incorrectWord, const vector<string>& list) {
    TopKTracker tracker;

    for (const auto& candidate : list) {
        float wLev, wKbd;
        computeDynamicWeights(incorrectWord, candidate, wLev, wKbd);

        float levScore = levenshtein(incorrectWord, candidate);
        float kbScore = keyboardDist(incorrectWord, candidate);

        float maxLen = static_cast<float>(max(incorrectWord.length(), candidate.length()));
        float normLev = (maxLen == 0.0f) ? 0.0f : (levScore / maxLen);

        float totalScore = (wLev * normLev) + (wKbd * kbScore);
        tracker.insert(totalScore, candidate, wLev, wKbd);
    }

    return tracker.getSortedResults();
}

// CSV Parser Helper____________________________________________________________________________________________________
vector<string> parseCSVLine(const string& line) {
    vector<string> fields;
    string field;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}
//_______________________________________________________________________________________________________________________
int main() {
    cout << "========================================================================\n";
    cout << "      SPELLCHECK BENCHMARK SUITE - ANALYTICS EVALUATOR\n";
    cout << "========================================================================\n";

    // 1. Load Lexicon
    vector<string> lexicon;
    set<string> lexiconSet;
    ifstream lexIn(LEXICON_FILE);
    if (!lexIn) {
        cerr << "Error: Could not open " << LEXICON_FILE << "!\n";
        return 1;
    }
    string w;
    while (lexIn >> w) {
        if (!w.empty()) {
            lexicon.push_back(w);
            lexiconSet.insert(w);
        }
    }
    lexIn.close();
    cout << "Loaded Lexicon Pool Size: " << lexicon.size() << " terms.\n";

    // 2. Load CSV Test Cases
    ifstream csvIn(TEST_CASES_CSV);
    if (!csvIn) {
        cerr << "Error: Could not open " << TEST_CASES_CSV << "!\n";
        return 1;
    }
    string header;
    getline(csvIn, header);

    vector<TestCase> testCases;
    string line;
    set<string> missingFromLex;

    while (getline(csvIn, line)) {
        if (line.empty()) continue;
        vector<string> f = parseCSVLine(line);
        if (f.size() < 7) continue;

        TestCase tc;
        tc.id          = stoi(f[0]);
        tc.query       = f[1];
        tc.groundTruth = f[2];
        tc.category    = f[3];
        tc.type        = f[4];
        tc.truthLen    = stoi(f[5]);
        tc.notes       = f[6];

        testCases.push_back(tc);

        // Verification of lexicon presence
        if (tc.groundTruth != "<NONE>" && !lexiconSet.count(tc.groundTruth)) {
            missingFromLex.insert(tc.groundTruth);
        }
    }
    csvIn.close();

    cout << "Loaded " << testCases.size() << " test cases from " << TEST_CASES_CSV << ".\n";
    if (missingFromLex.empty()) {
        cout << "[VERIFIED] All valid ground-truth words exist in the candidate list!\n";
    } else {
        cout << "[WARNING] Missing " << missingFromLex.size() << " ground truth words from lexicon!\n";
    }

    // 3. Run Benchmark
    cout << "Executing recommendations across all test cases...\n";
    vector<QueryResult> results;
    results.reserve(testCases.size());

    for (const auto& tc : testCases) {
        auto tStart = chrono::high_resolution_clock::now();
        vector<Node> topk = recomendation(tc.query, lexicon);
        auto tEnd = chrono::high_resolution_clock::now();

        double lat = chrono::duration<double, micro>(tEnd - tStart).count();

        int matchedRank = 9999;
        float bScore = 1.0f;
        float wLevUsed = 0.50f, wKbdUsed = 0.50f;

        bool inLex = (tc.groundTruth != "<NONE>");
        if (inLex) {
            computeDynamicWeights(tc.query, tc.groundTruth, wLevUsed, wKbdUsed);
            for (size_t i = 0; i < topk.size(); ++i) {
                if (topk[i].word == tc.groundTruth) {
                    matchedRank = static_cast<int>(i + 1);
                    bScore = topk[i].score;
                    wLevUsed = topk[i].wLev;
                    wKbdUsed = topk[i].wKbd;
                    break;
                }
            }
        }

        if (matchedRank == 9999 && !topk.empty()) {
            bScore = topk[0].score;
        }

        results.push_back({
            tc,
            matchedRank,
            bScore,
            wLevUsed,
            wKbdUsed,
            lat,
            inLex,
            (matchedRank == 1),
            (matchedRank <= K)
        });
    }

    // 4. Export Detailed Analytics Report
    ofstream ana(ANALYTICS_FILE);
    ana << "=========================================================================================\n";
    ana << "                  SPELLCHECK ADAPTIVE ENGINE - DETAILED ANALYTICS\n";
    ana << "=========================================================================================\n";
    ana << "Total Test Samples Evaluated : " << testCases.size() << "\n";
    ana << "Candidate Lexicon Pool Size  : " << lexicon.size() << " terms\n";
    ana << "Dynamic Weights Configuration: init=" << init << ", damper=" << damper 
        << ", boost=" << boost << ", lowBnd=" << lowBnd << ", upBnd=" << upBnd << "\n\n";

    ana << "-----------------------------------------------------------------------------------------\n";
    ana << "PART 1: CATEGORY PERFORMANCE MATRIX (EXCLUDING OUT-OF-VOCABULARY EDGES)\n";
    ana << "-----------------------------------------------------------------------------------------\n";
    ana << left << setw(30) << "Category"
        << setw(9)  << "Valid N"
        << setw(12) << "Top-1 Acc"
        << setw(12) << "Top-K Acc"
        << setw(10) << "MRR"
        << setw(10) << "Avg W_lev"
        << setw(10) << "Avg W_kbd"
        << "Avg Lat (us)\n";
    ana << string(103, '-') << "\n";

    vector<string> cats = {
        "Adjacent Horizontal Slips",
        "Vertical/Diagonal Slips",
        "Single Deletions",
        "Single Insertions",
        "Structural & Phonetic"
    };

    int validTotal = 0, gTop1 = 0, gTop5 = 0;
    double gMrr = 0.0, gLat = 0.0, gWlev = 0.0, gWkbd = 0.0;

    for (const auto& c : cats) {
        int cnt = 0, t1 = 0, t5 = 0;
        double mrr = 0.0, lat = 0.0, sWl = 0.0, sWk = 0.0;

        for (const auto& r : results) {
            if (r.tc.category == c && r.inLexicon) {
                cnt++;
                if (r.passedTop1) t1++;
                if (r.passedTop5) t5++;
                if (r.rank <= K) mrr += (1.0 / static_cast<double>(r.rank));
                sWl += r.wLev;
                sWk += r.wKbd;
                lat += r.latencyUs;
            }
        }

        double cTop1 = (cnt > 0) ? (static_cast<double>(t1) / cnt * 100.0) : 0.0;
        double cTop5 = (cnt > 0) ? (static_cast<double>(t5) / cnt * 100.0) : 0.0;
        double cMrr  = (cnt > 0) ? (mrr / cnt) : 0.0;

        validTotal += cnt;
        gTop1      += t1;
        gTop5      += t5;
        gMrr       += mrr;
        gLat       += lat;
        gWlev      += sWl;
        gWkbd      += sWk;

        ana << left << setw(30) << c
            << setw(9)  << cnt
            << fixed << setprecision(1) << setw(12) << (to_string((int)cTop1) + "%")
            << setw(12) << (to_string((int)cTop5) + "%")
            << setprecision(3) << setw(10) << cMrr
            << setprecision(2) << setw(10) << (cnt ? sWl / cnt : 0.0)
            << setw(10) << (cnt ? sWk / cnt : 0.0)
            << setprecision(1) << (cnt ? lat / cnt : 0.0) << "\n";
    }

    double finalTop1 = validTotal ? (static_cast<double>(gTop1) / validTotal * 100.0) : 0.0;
    double finalTop5 = validTotal ? (static_cast<double>(gTop5) / validTotal * 100.0) : 0.0;
    double finalMrr  = validTotal ? (gMrr / validTotal) : 0.0;
    double finalLat  = validTotal ? (gLat / validTotal) : 0.0;

    ana << "\n-----------------------------------------------------------------------------------------\n";
    ana << "PART 2: GLOBAL BENCHMARK SUMMARY\n";
    ana << "-----------------------------------------------------------------------------------------\n";
    ana << "In-Lexicon Valid Queries Run    : " << validTotal << " / 600\n";
    ana << "EDGE / Out-of-Lexicon Queries   : " << (testCases.size() - validTotal) << " (rejected as intended)\n";
    ana << "Global Top-1 Accuracy           : " << fixed << setprecision(2) << finalTop1 << "%\n";
    ana << "Global Top-K Accuracy           : " << fixed << setprecision(2) << finalTop5 << "%\n";
    ana << "Global Mean Reciprocal Rank(MRR): " << setprecision(4) << finalMrr << "\n";
    ana << "Average Pipeline Latency        : " << setprecision(2) << finalLat << " us\n";
    ana << "Average Weight Balance          : W_lev = " << setprecision(2) << (gWlev / validTotal)
        << " | W_kbd = " << (gWkbd / validTotal) << "\n";

    ana << "\n-----------------------------------------------------------------------------------------\n";
    ana << "PART 3: INDIVIDUAL QUERY TRACE (ALL 600 SAMPLES)\n";
    ana << "-----------------------------------------------------------------------------------------\n";
    ana << left << setw(6)  << "ID"
        << setw(18) << "Query"
        << setw(18) << "Ground Truth"
        << setw(8)  << "Rank"
        << setw(12) << "Score"
        << setw(8)  << "W_lev"
        << setw(8)  << "W_kbd"
        << setw(10) << "Type"
        << "Status\n";
    ana << string(94, '-') << "\n";

    for (const auto& r : results) {
        string rankStr = (!r.inLexicon) ? "OOV" : ((r.rank <= K) ? ("#" + to_string(r.rank)) : "FAIL");
        string statStr = (!r.inLexicon) ? "REJECTED" : (r.passedTop1 ? "TOP-1" : (r.passedTop5 ? "Top-K" : "MISS"));

        ana << left << setw(6)  << r.tc.id
            << setw(18) << r.tc.query
            << setw(18) << r.tc.groundTruth
            << setw(8)  << rankStr
            << fixed << setprecision(4) << setw(12) << r.bestScore
            << setprecision(2) << setw(8) << r.wLev
            << setw(8) << r.wKbd
            << setw(10) << r.tc.type
            << statStr << "\n";
    }
    ana.close();

    cout << "\nAnalytics complete!\n";
    cout << "Global Top-1 Accuracy: " << finalTop1 << "% | Top-K Accuracy: " << finalTop5 << "% | MRR: " << finalMrr << "\n";
    cout << "Results written to: " << ANALYTICS_FILE << "\n";

    return 0;
}