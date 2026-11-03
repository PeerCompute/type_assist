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
#include <map>

using namespace std;

#define K 3
#define MAX_WORD_LEN 32

const string TEST_CASES_CSV = "test_cases_960.csv";
const string LEXICON_FILE    = "lexicon_600.txt";
const string ANALYTICS_FILE  = "hybrid_optimal_analytics_960.txt";

// Alpha: Weight for physical keyboard tie-breaking
constexpr float ALPHA = 0.03f;

struct XY { float x, y; };

static const XY KB_MAP[26] = {
    {0.8f, 1.0f}, {5.2f, 2.0f}, {3.2f, 2.0f}, {2.8f, 1.0f}, {2.5f, 0.0f},
    {3.8f, 1.0f}, {4.8f, 1.0f}, {5.8f, 1.0f}, {7.5f, 0.0f}, {6.8f, 1.0f},
    {7.8f, 1.0f}, {8.8f, 1.0f}, {7.2f, 2.0f}, {6.2f, 2.0f}, {8.5f, 0.0f},
    {9.5f, 0.0f}, {0.5f, 0.0f}, {3.5f, 0.0f}, {1.8f, 1.0f}, {4.5f, 0.0f},
    {6.5f, 0.0f}, {4.2f, 2.0f}, {1.5f, 0.0f}, {2.2f, 2.0f}, {5.5f, 0.0f},
    {1.2f, 2.0f}
};

static inline float charKbDist(char a, char b) {
    if (a == b) return 0.0f;
    a = static_cast<char>(tolower(static_cast<unsigned char>(a)));
    b = static_cast<char>(tolower(static_cast<unsigned char>(b)));
    if (a < 'a' || a > 'z' || b < 'a' || b > 'z') return 1.0f;
    const XY& p1 = KB_MAP[a - 'a'];
    const XY& p2 = KB_MAP[b - 'a'];
    float dx = p2.x - p1.x, dy = p2.y - p1.y;
    return min(1.0f, sqrt(dx * dx + dy * dy) / 4.0f);
}

// Full Damerau-Levenshtein (Solves the 2-Letter Swap Deficit)
int damerauLevenshtein(const string& s1, const string& s2) {
    int m = static_cast<int>(s1.length());
    int n = static_cast<int>(s2.length());
    if (m > MAX_WORD_LEN || n > MAX_WORD_LEN) return max(m, n);

    int d[MAX_WORD_LEN + 1][MAX_WORD_LEN + 1];
    for (int i = 0; i <= m; ++i) d[i][0] = i;
    for (int j = 0; j <= n; ++j) d[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (tolower(static_cast<unsigned char>(s1[i - 1])) == 
                        tolower(static_cast<unsigned char>(s2[j - 1]))) ? 0 : 1;
            
            d[i][j] = min({ d[i - 1][j] + 1,        // deletion
                            d[i][j - 1] + 1,        // insertion
                            d[i - 1][j - 1] + cost }); // substitution

            // Adjacent transposition check
            if (i > 1 && j > 1 &&
                tolower(static_cast<unsigned char>(s1[i - 1])) == tolower(static_cast<unsigned char>(s2[j - 2])) &&
                tolower(static_cast<unsigned char>(s1[i - 2])) == tolower(static_cast<unsigned char>(s2[j - 1]))) {
                d[i][j] = min(d[i][j], d[i - 2][j - 2] + 1);
            }
        }
    }
    return d[m][n];
}

float keyboardDist(const string& s1, const string& s2) {
    int l1 = static_cast<int>(s1.length()), l2 = static_cast<int>(s2.length());
    if (abs(l1 - l2) > 2) return 1.0f;
    int minLen = min(l1, l2), maxLen = max(l1, l2);
    if (maxLen == 0) return 0.0f;
    float score = 0.0f;
    for (int i = 0; i < minLen; ++i) score += charKbDist(s1[i], s2[i]);
    score += static_cast<float>(maxLen - minLen);
    return score / static_cast<float>(maxLen);
}

struct Node { float score; string word; };

class TopKTracker {
private:
    Node heap[K];
    int count;
    void heapifyUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (heap[i].score > heap[p].score) { swap(heap[i], heap[p]); i = p; }
            else break;
        }
    }
    void heapifyDown(int i) {
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, largest = i;
            if (l < count && heap[l].score > heap[largest].score) largest = l;
            if (r < count && heap[r].score > heap[largest].score) largest = r;
            if (largest != i) { swap(heap[i], heap[largest]); i = largest; }
            else break;
        }
    }
public:
    TopKTracker() : count(0) {}
    void insert(float score, const string& word) {
        if (count < K) { heap[count] = {score, word}; heapifyUp(count++); }
        else if (score < heap[0].score) { heap[0] = {score, word}; heapifyDown(0); }
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
    float totalScore;
    int rank;
    double timeMs;
    string category;
    bool passed;
};

vector<string> parseCSVLine(const string& line) {
    vector<string> fields;
    string field;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') { field += '"'; ++i; }
            else inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else field += c;
    }
    fields.push_back(field);
    return fields;
}

int main() {
    vector<string> lexicon;
    ifstream lexIn(LEXICON_FILE);
    string w;
    while (lexIn >> w) if (!w.empty()) lexicon.push_back(w);
    lexIn.close();

    ifstream csvIn(TEST_CASES_CSV);
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

    vector<QueryResult> results;
    results.reserve(testCases.size());

    for (const auto& tc : testCases) {
        auto t0 = chrono::high_resolution_clock::now();
        TopKTracker tracker;

        for (const auto& cand : lexicon) {
            int dLev = damerauLevenshtein(tc.incorectWord, cand);
            float maxL = static_cast<float>(max(tc.incorectWord.length(), cand.length()));
            float normDLev = (maxL == 0.0f) ? 0.0f : (static_cast<float>(dLev) / maxL);
            float kDist = keyboardDist(tc.incorectWord, cand);
            
            // Structural Alignment + Keyboard Spatial Affinity
            float composite = normDLev + (ALPHA * kDist);
            tracker.insert(composite, cand);
        }

        vector<Node> topk = tracker.getSortedResults();
        auto t1 = chrono::high_resolution_clock::now();
        double ms = chrono::duration<double, milli>(t1 - t0).count();

        int matchedRank = 9999;
        float score = 1.0f;

        for (size_t i = 0; i < topk.size(); ++i) {
            if (topk[i].word == tc.corectWord) {
                matchedRank = static_cast<int>(i + 1);
                score = topk[i].score;
                break;
            }
        }

        bool passed = (matchedRank <= K);
        results.push_back({tc.id, tc.incorectWord, tc.corectWord, score, matchedRank, ms, tc.category, passed});
    }

    // Export report
    ofstream out(ANALYTICS_FILE);
    out << "========================================================================================================================\n";
    out << "                          OPTIMAL HYBRID BENCHMARK LOG (DAMERAU-LEVENSHTEIN + QWERTY)\n";
    out << "========================================================================================================================\n";
    out << "Candidate Lexicon Size : " << lexicon.size() << " terms\n";
    out << "Top-K Evaluated        : " << K << "\n";
    out << "Architecture           : Damerau-Levenshtein DP + Spatial QWERTY Tie-Breaker (Alpha = " << ALPHA << ")\n\n";

    out << "------------------------------------------------------------------------------------------------------------------------\n";
    out << "PART 1: INDIVIDUAL QUERY TRACE RECORDS\n";
    out << "Format: id, wrongWrd, CorrectWrd, composite_score, rank among top 3, time in milisec, category, pass/fail\n";
    out << "------------------------------------------------------------------------------------------------------------------------\n";
    out << left << setw(5)  << "id"
        << setw(18) << "wrongWrd"
        << setw(18) << "CorrectWrd"
        << setw(18) << "composite_score"
        << setw(12) << "rank_top3"
        << setw(14) << "time_ms"
        << setw(42) << "category"
        << "pass/fail\n";
    out << string(120, '-') << "\n";

    for (const auto& r : results) {
        string rankStr = (r.rank <= K) ? ("#" + to_string(r.rank)) : "NONE";
        out << left << setw(5)  << r.id
            << setw(18) << r.wrongWrd
            << setw(18) << r.correctWrd
            << fixed << setprecision(4) << setw(18) << r.totalScore
            << setw(12) << rankStr
            << setprecision(3) << setw(14) << r.timeMs
            << setw(42) << r.category
            << (r.passed ? "pass" : "fail") << "\n";
    }

    int totalEvaluated = results.size(), totalPassed = 0, totalTop1 = 0;
    double totalMrr = 0.0, totalTime = 0.0;
    map<string, int> catTotal, catPassed, catTop1;
    map<string, double> catMrr, catTime;

    for (const auto& r : results) {
        catTotal[r.category]++;
        catTime[r.category] += r.timeMs;
        totalTime += r.timeMs;
        if (r.passed) {
            totalPassed++;
            catPassed[r.category]++;
            totalMrr += 1.0 / r.rank;
            catMrr[r.category] += 1.0 / r.rank;
        }
        if (r.rank == 1) { totalTop1++; catTop1[r.category]++; }
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
        << "Avg Time(ms)\n";
    out << string(95, '-') << "\n";

    for (const auto& p : catTotal) {
        string cat = p.first;
        int tot = p.second, pPass = catPassed[cat], t1 = catTop1[cat];
        double mrr = catMrr[cat];
        out << left << setw(44) << cat
            << setw(9)  << tot
            << setw(11) << t1
            << setw(11) << pPass
            << fixed << setprecision(2) << setw(12) << (to_string((int)(100.0 * pPass / tot)) + "%")
            << setprecision(4) << setw(10) << (mrr / tot)
            << setprecision(3) << (catTime[cat] / tot) << "\n";
    }

    out << "\n========================================================================================================================\n";
    out << "PART 3: GLOBAL OVERALL BENCHMARK ANALYTICS\n";
    out << "========================================================================================================================\n";
    out << "Total Test Cases Evaluated     : " << totalEvaluated << " records\n";
    out << "Global Top-1 Matches (Rank #1) : " << totalTop1 << " / " << totalEvaluated 
        << " (" << fixed << setprecision(2) << (100.0 * totalTop1 / totalEvaluated) << "%)\n";
    out << "Global Top-3 Passes (Rank <=3) : " << totalPassed << " / " << totalEvaluated 
        << " (" << fixed << setprecision(2) << (100.0 * totalPassed / totalEvaluated) << "%)\n";
    out << "Global Fails (Rank > 3)        : " << (totalEvaluated - totalPassed) << " / " << totalEvaluated 
        << " (" << fixed << setprecision(2) << (100.0 * (totalEvaluated - totalPassed) / totalEvaluated) << "%)\n";
    out << "Global Mean Reciprocal Rank MRR: " << setprecision(4) << (totalMrr / totalEvaluated) << "\n";
    out << "Average Query Latency          : " << setprecision(3) << (totalTime / totalEvaluated) << " ms per query\n";
    out << "========================================================================================================================\n";
    out.close();

    cout << "Optimal Hybrid complete! Top-1: " << 100.0 * totalTop1 / totalEvaluated 
         << "% | Top-3: " << 100.0 * totalPassed / totalEvaluated << "%\n";
    return 0;
}