
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

// constexpr float upBnd   = 0.70f;
// constexpr float lowBnd  = 0.05f;
// constexpr float damper  = 0.035f;
// constexpr float boost   = 0.11f;
// constexpr float init    = 0.50f;

constexpr float upBnd   = 0.6f;
constexpr float lowBnd  = 0.1f;
constexpr float damper  = 0.035f;
constexpr float boost   = 0.08f;
constexpr float init    = 0.5f;

const string TEST_CASES_CSV = "test_cases_960.csv";
const string LEXICON_FILE    = "lexicon_600.txt";
const string ANALYTICS_FILE  = "hyb_analytics_960.txt";

struct XY { float x, y; };

struct Node {
    float score;
    string word;
    float wLev;
    float wKbd;
};

struct TestCase {
    int id;
    string incorectWord;
    string corectWord;
    string category;
    string scenario;
};

struct QueryResult {
    int id;
    string wrongWrd;
    string correctWrd;
    float wLev;
    float wKbd;
    int rank;
    float score;
    double timeMs;
    string category;
    bool passed;
};

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

    if (abs(l1 - l2) > 2) return 1.0f;

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

    float lengthDiscrepancy = static_cast<float>(lenDiff) / static_cast<float>(maxLen);
    float kbdWeight = init * (1.0f - lengthDiscrepancy);

    if (minLen > 5) {
        kbdWeight *= (1.0f - damper * static_cast<float>(minLen - 5));
    }

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
            } else inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else field += c;
    }
    fields.push_back(field);
    return fields;
}

int main() {
    // 1. Load Lexicon
    vector<string> lexicon;
    ifstream lexIn(LEXICON_FILE);
    if (!lexIn) {
        cerr << "Error: Could not open " << LEXICON_FILE << "\n";
        return 1;
    }
    string w;
    while (lexIn >> w) {
        if (!w.empty()) lexicon.push_back(w);
    }
    lexIn.close();

    // 2. Load test_cases_960.csv
    ifstream csvIn(TEST_CASES_CSV);
    if (!csvIn) {
        cerr << "Error: Could not open " << TEST_CASES_CSV << "\n";
        return 1;
    }
    string line, header;
    getline(csvIn, header);
    vector<TestCase> testCases;
    while (getline(csvIn, line)) {
        if (line.empty()) continue;
        vector<string> f = parseCSVLine(line);
        if (f.size() < 5) continue;
        testCases.push_back({stoi(f[0]), f[1], f[2], f[3], f[4]});
    }
    csvIn.close();

    cout << "Loaded " << lexicon.size() << " lexicon words.\n";
    cout << "Loaded " << testCases.size() << " test cases.\n";

    // 3. Process each query
    vector<QueryResult> results;
    results.reserve(testCases.size());

    for (const auto& tc : testCases) {
        auto t0 = chrono::high_resolution_clock::now();
        TopKTracker tracker;

        for (const auto& cand : lexicon) {
            float wL, wK;
            computeDynamicWeights(tc.incorectWord, cand, wL, wK);
            float lScore = levenshtein(tc.incorectWord, cand);
            float kScore = keyboardDist(tc.incorectWord, cand);
            float maxL = static_cast<float>(max(tc.incorectWord.length(), cand.length()));
            float normL = (maxL == 0.0f) ? 0.0f : (lScore / maxL);
            float tot = (wL * normL) + (wK * kScore);
            tracker.insert(tot, cand, wL, wK);
        }

        vector<Node> topk = tracker.getSortedResults();
        auto t1 = chrono::high_resolution_clock::now();
        double ms = chrono::duration<double, milli>(t1 - t0).count();

        int matchedRank = 9999;
        float bScore = 1.0f;
        float wLevPair = 0.5f, wKbdPair = 0.5f;

        computeDynamicWeights(tc.incorectWord, tc.corectWord, wLevPair, wKbdPair);

        for (size_t i = 0; i < topk.size(); ++i) {
            if (topk[i].word == tc.corectWord) {
                matchedRank = static_cast<int>(i + 1);
                bScore = topk[i].score;
                break;
            }
        }

        if (matchedRank == 9999) {
            float lScore = levenshtein(tc.incorectWord, tc.corectWord);
            float kScore = keyboardDist(tc.incorectWord, tc.corectWord);
            float maxL = static_cast<float>(max(tc.incorectWord.length(), tc.corectWord.length()));
            float normL = (maxL == 0.0f) ? 0.0f : (lScore / maxL);
            bScore = (wLevPair * normL) + (wKbdPair * kScore);
        }

        bool passed = (matchedRank <= K);
        results.push_back({
            tc.id,
            tc.incorectWord,
            tc.corectWord,
            wLevPair,
            wKbdPair,
            matchedRank,
            bScore,
            ms,
            tc.category,
            passed
        });
    }

    // 4. Output analytics_960.txt
    ofstream out(ANALYTICS_FILE);

    out << "========================================================================================================================\n";
    out << "                                        SPELLCHECK BENCHMARK LOG (960 TESTCASES)\n";
    out << "========================================================================================================================\n";
    out << "Candidate Lexicon Size : " << lexicon.size() << " terms\n";
    out << "Top-K Evaluated        : " << K << "\n";
    out << "Evaluation Engine      : Dynamic Hybrid (Levenshtein + QWERTY Euclidean Keyboard Distance)\n\n";

    out << "------------------------------------------------------------------------------------------------------------------------\n";
    out << "PART 1: INDIVIDUAL QUERY TRACE RECORDS\n";
    out << "Format: id, wrongWrd, CorrectWrd, weight of levnstien, weight of kbdist, rank among top 3, time in milisec, category, pass/fail\n";
    out << "------------------------------------------------------------------------------------------------------------------------\n";
    out << left << setw(5)  << "id"
        << setw(18) << "wrongWrd"
        << setw(18) << "CorrectWrd"
        << setw(13) << "w_lev"
        << setw(13) << "w_kbd"
        << setw(12) << "rank_top3"
        << setw(14) << "time_ms"
        << setw(42) << "category"
        << "pass/fail\n";
    out << string(120, '-') << "\n";

    for (const auto& r : results) {
        string rankStr = (r.rank <= K) ? ("#" + to_string(r.rank)) : "NONE";
        string statusStr = r.passed ? "pass" : "fail";

        out << left << setw(5)  << r.id
            << setw(18) << r.wrongWrd
            << setw(18) << r.correctWrd
            << fixed << setprecision(4) << setw(13) << r.wLev
            << setw(13) << r.wKbd
            << setw(12) << rankStr
            << setprecision(3) << setw(14) << r.timeMs
            << setw(42) << r.category
            << statusStr << "\n";
    }

    // Summary calculations
    int totalEvaluated = results.size();
    int totalPassed = 0;
    int totalTop1 = 0;
    double totalMrr = 0.0;
    double totalTime = 0.0;

    map<string, int> catTotal;
    map<string, int> catPassed;
    map<string, int> catTop1;
    map<string, double> catMrr;
    map<string, double> catTime;
    map<string, double> catWlev;
    map<string, double> catWkbd;

    for (const auto& r : results) {
        catTotal[r.category]++;
        catTime[r.category] += r.timeMs;
        catWlev[r.category] += r.wLev;
        catWkbd[r.category] += r.wKbd;
        totalTime += r.timeMs;

        if (r.passed) {
            totalPassed++;
            catPassed[r.category]++;
            double rr = 1.0 / r.rank;
            totalMrr += rr;
            catMrr[r.category] += rr;
        }
        if (r.rank == 1) {
            totalTop1++;
            catTop1[r.category]++;
        }
    }

    out << "\n========================================================================================================================\n";
    out << "PART 2: CATEGORY-WISE STATISTICAL PERFORMANCE BREAKDOWN\n";
    out << "========================================================================================================================\n";
    out << left << setw(44) << "Category"
        << setw(9)  << "Samples"
        << setw(11) << "Top-1"
        << setw(11) << "Top-3"
        << setw(12) << "Top-3 Acc"
        << setw(10) << "MRR"
        << setw(12) << "Avg W_lev"
        << setw(12) << "Avg W_kbd"
        << "Avg Time(ms)\n";
    out << string(120, '-') << "\n";

    vector<string> catOrder = {
        "1 letter insertion",
        "1 letter deletion",
        "1 letter missspell(same row nearby keys)",
        "1 letter missspell(same row faraway keys)",
        "1 letter missspell(diff row nearby keys)",
        "1 letter missspell(diff row faraway keys)",
        "2 letter insertion",
        "2 letter deletion",
        "2 letter missspell",
        "2 letter swap",
        "3 letter insertion",
        "3 letter deletion",
        "3 letter missspell",
        "4 letter insertion",
        "4 letter deletion",
        "4 letter missspell"
    };

    for (const auto& cat : catOrder) {
        int tot = catTotal[cat];
        int p = catPassed[cat];
        int t1 = catTop1[cat];
        double mrr = catMrr[cat];
        double acc = (tot > 0) ? (100.0 * p / tot) : 0.0;
        double avgT = (tot > 0) ? (catTime[cat] / tot) : 0.0;
        double avgWl = (tot > 0) ? (catWlev[cat] / tot) : 0.0;
        double avgWk = (tot > 0) ? (catWkbd[cat] / tot) : 0.0;

        out << left << setw(44) << cat
            << setw(9)  << tot
            << setw(11) << t1
            << setw(11) << p
            << fixed << setprecision(2) << setw(12) << (to_string((int)acc) + "%")
            << setprecision(4) << setw(10) << (mrr / tot)
            << setprecision(3) << setw(12) << avgWl
            << setw(12) << avgWk
            << setprecision(3) << avgT << "\n";
    }

    out << "\n========================================================================================================================\n";
    out << "PART 3: GLOBAL OVERALL BENCHMARK ANALYTICS\n";
    out << "========================================================================================================================\n";
    out << "Total Test Cases Evaluated     : " << totalEvaluated << " records across 16 categories\n";
    out << "Global Top-1 Matches (Rank #1) : " << totalTop1 << " / " << totalEvaluated 
        << " (" << fixed << setprecision(2) << (100.0 * totalTop1 / totalEvaluated) << "%)\n";
    out << "Global Top-3 Passes (Rank <=3) : " << totalPassed << " / " << totalEvaluated 
        << " (" << fixed << setprecision(2) << (100.0 * totalPassed / totalEvaluated) << "%)\n";
    out << "Global Fails (Rank > 3)        : " << (totalEvaluated - totalPassed) << " / " << totalEvaluated 
        << " (" << fixed << setprecision(2) << (100.0 * (totalEvaluated - totalPassed) / totalEvaluated) << "%)\n";
    out << "Global Mean Reciprocal Rank MRR: " << setprecision(4) << (totalMrr / totalEvaluated) << "\n";
    out << "Total Benchmark Time Elapsed   : " << setprecision(2) << totalTime << " ms (" << (totalTime / 1000.0) << " seconds)\n";
    out << "Average Query Latency          : " << setprecision(3) << (totalTime / totalEvaluated) << " ms per query (" 
        << setprecision(1) << (totalTime * 1000.0 / totalEvaluated) << " microseconds)\n";
    out << "========================================================================================================================\n";

    out.close();
    cout << "Benchmark complete! Analytics saved to " << ANALYTICS_FILE << "\n";
    return 0;
}
