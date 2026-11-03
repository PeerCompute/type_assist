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

const string TEST_CASES_CSV = "new_test_cases_600.csv";
const string LEXICON_FILE    = "lexicon_600.txt";
const string OUTPUT_REPORT   = "levenshtein_analytics.txt";

struct TestCase {
    int id;
    string query;
    string groundTruth;
    string category;
    string type;
    int truthLen;
    string notes;
};

struct Node {
    float score;
    int rawEdit;
    string word;
};

struct ResultRecord {
    TestCase tc;
    int rank;
    float normScore;
    int rawEdit;
    double latencyUs;
    bool inLexicon;
    bool passedTop1;
    bool passedTopK;
};

int levenshtein(const string& s1, const string& s2) {
    int m = static_cast<int>(s1.length());
    int n = static_cast<int>(s2.length());

    if (m > MAX_WORD_LEN || n > MAX_WORD_LEN) return max(m, n);

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
    return dp[n];
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

    void insert(float score, int rawEdit, const string& word) {
        if (count < K) {
            heap[count] = {score, rawEdit, word};
            heapifyUp(count);
            count++;
        } else if (score < heap[0].score) {
            heap[0] = {score, rawEdit, word};
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
        if (f.size() < 7) continue;
        testCases.push_back({stoi(f[0]), f[1], f[2], f[3], f[4], stoi(f[5]), f[6]});
    }
    csvIn.close();

    vector<ResultRecord> results;
    results.reserve(testCases.size());

    int validN = 0, top1Count = 0, topKCount = 0;
    double totalMrr = 0.0, totalLatency = 0.0;
    map<string, int> catTotal, catTop1, catTopK;
    map<string, double> catMrr, catLat;

    for (const auto& tc : testCases) {
        auto t0 = chrono::high_resolution_clock::now();
        TopKTracker tracker;

        for (const auto& cand : lexicon) {
            int rawDist = levenshtein(tc.query, cand);
            float maxLen = static_cast<float>(max(tc.query.length(), cand.length()));
            float normScore = (maxLen == 0.0f) ? 0.0f : (static_cast<float>(rawDist) / maxLen);
            tracker.insert(normScore, rawDist, cand);
        }

        vector<Node> topk = tracker.getSortedResults();
        auto t1 = chrono::high_resolution_clock::now();
        double lat = chrono::duration<double, micro>(t1 - t0).count();

        bool inLex = (tc.groundTruth != "<NONE>");
        int matchedRank = 9999;
        float finalScore = 1.0f;
        int finalRaw = 999;

        if (inLex) {
            for (size_t i = 0; i < topk.size(); ++i) {
                if (topk[i].word == tc.groundTruth) {
                    matchedRank = static_cast<int>(i + 1);
                    finalScore = topk[i].score;
                    finalRaw = topk[i].rawEdit;
                    break;
                }
            }
            if (matchedRank == 9999) {
                finalRaw = levenshtein(tc.query, tc.groundTruth);
                float maxLen = static_cast<float>(max(tc.query.length(), tc.groundTruth.length()));
                finalScore = (maxLen == 0.0f) ? 0.0f : (static_cast<float>(finalRaw) / maxLen);
            }

            validN++;
            totalLatency += lat;
            catTotal[tc.category]++;
            catLat[tc.category] += lat;

            if (matchedRank == 1) {
                top1Count++;
                catTop1[tc.category]++;
            }
            if (matchedRank <= K) {
                topKCount++;
                catTopK[tc.category]++;
                double rr = 1.0 / matchedRank;
                totalMrr += rr;
                catMrr[tc.category] += rr;
            }
        } else if (!topk.empty()) {
            finalScore = topk[0].score;
            finalRaw = topk[0].rawEdit;
        }

        results.push_back({
            tc, matchedRank, finalScore, finalRaw, lat,
            inLex, (matchedRank == 1), (matchedRank <= K)
        });
    }

    ofstream out(OUTPUT_REPORT);
    out << "=========================================================================================\n";
    out << "            LEVENSHTEIN-ONLY EVALUATION REPORT (K = " << K << ")\n";
    out << "=========================================================================================\n";
    out << "Reference Lexicon Size  : " << lexicon.size() << " words\n";
    out << "Total Test Cases        : " << testCases.size() << "\n";
    out << "In-Lexicon Valid Cases  : " << validN << "\n";
    out << "EDGE / OOV Cases        : " << (testCases.size() - validN) << " (rejected)\n";
    out << "Top-1 Accuracy          : " << fixed << setprecision(2) << (100.0 * top1Count / validN) << "%\n";
    out << "Top-" << K << " Accuracy          : " << fixed << setprecision(2) << (100.0 * topKCount / validN) << "%\n";
    out << "Mean Reciprocal Rank    : " << setprecision(4) << (totalMrr / validN) << "\n";
    out << "Average Latency         : " << setprecision(1) << (totalLatency / validN) << " us\n\n";

    out << "-----------------------------------------------------------------------------------------\n";
    out << "CATEGORY PERFORMANCE SUMMARY (K = " << K << ")\n";
    out << "-----------------------------------------------------------------------------------------\n";
    out << left << setw(30) << "Category"
        << setw(8)  << "Valid"
        << setw(12) << "Top-1 Acc"
        << setw(12) << ("Top-" + to_string(K) + " Acc")
        << setw(10) << "MRR"
        << "Avg Lat (us)\n";
    out << string(80, '-') << "\n";

    for (const auto& p : catTotal) {
        string cat = p.first;
        int tot = p.second;
        int t1 = catTop1.count(cat) ? catTop1.at(cat) : 0;
        int tk = catTopK.count(cat) ? catTopK.at(cat) : 0;
        double mrr = catMrr.count(cat) ? catMrr.at(cat) : 0.0;
        double lat = catLat.count(cat) ? catLat.at(cat) : 0.0;

        out << left << setw(30) << cat
            << setw(8)  << tot
            << fixed << setprecision(1) << setw(12) << (to_string((int)(100.0 * t1 / tot)) + "%")
            << setw(12) << (to_string((int)(100.0 * tk / tot)) + "%")
            << setprecision(3) << setw(10) << (mrr / tot)
            << setprecision(1) << (lat / tot) << "\n";
    }

    out << "\n-----------------------------------------------------------------------------------------\n";
    out << "INDIVIDUAL QUERY TRACE LOG (ALL " << testCases.size() << " CASES)\n";
    out << "-----------------------------------------------------------------------------------------\n";
    out << left << setw(6)  << "ID"
        << setw(20) << "Query"
        << setw(20) << "Ground Truth"
        << setw(8)  << "Rank"
        << setw(12) << "Norm Score"
        << setw(10) << "Raw Edit"
        << setw(14) << "Latency (us)"
        << setw(10) << "Type"
        << "Status\n";
    out << string(106, '-') << "\n";

    for (const auto& r : results) {
        string rankStr = (!r.inLexicon) ? "OOV" : ((r.rank <= K) ? ("#" + to_string(r.rank)) : "FAIL");
        string statStr = (!r.inLexicon) ? "REJECTED" : (r.passedTop1 ? "TOP-1" : (r.passedTopK ? ("TOP-" + to_string(K)) : "MISS"));

        out << left << setw(6)  << r.tc.id
            << setw(20) << r.tc.query
            << setw(20) << r.tc.groundTruth
            << setw(8)  << rankStr
            << fixed << setprecision(4) << setw(12) << r.normScore
            << setw(10) << r.rawEdit
            << setprecision(1) << setw(14) << r.latencyUs
            << setw(10) << r.tc.type
            << statStr << "\n";
    }
    out.close();

    cout << "Levenshtein stats written to " << OUTPUT_REPORT << "\n";
    return 0;
}