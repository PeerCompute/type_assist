#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <cctype>

using namespace std;

#define K 5
#define MAX_WORD_LEN 32

// Configurable Dynamic Weights Parameters
constexpr float upBnd   = 0.70f;
constexpr float lowBnd  = 0.05f;
constexpr float damper  = 0.035f;
constexpr float boost   = 0.11f;
constexpr float init    = 0.50f;

const string TEST_CASES_FILE   = "test_cases_600.txt";
const string STATS_REPORT_FILE = "test_stats_report_600.txt";

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
    string note;
};

struct QueryResult {
    TestCase tc;
    int rank;
    float bestScore;
    float wLev;
    float wKbd;
    double latencyUs;
    bool passed;
};

// 2D QWERTY Coordinate Map
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

// -----------------------------------------------------------------------------
// 600 Developer Words Lexicon Sorted by Length
// -----------------------------------------------------------------------------
const vector<string> dictionary = {
    // Length 2 (15)
    "as", "at", "by", "do", "go", "id", "if", "in", "io", "is",
    "it", "no", "ok", "on", "to",
    // Length 3 (35)
    "and", "api", "app", "arg", "bin", "bit", "box", "bug", "cli", "col",
    "cpu", "cur", "dev", "dir", "doc", "dot", "end", "env", "err", "fit",
    "for", "get", "git", "hex", "int", "key", "len", "log", "map", "net",
    "new", "ptr", "raw", "row", "run",
    // Length 4 (65)
    "args", "auth", "auto", "base", "bind", "bool", "byte", "call", "case", "char",
    "code", "copy", "core", "cost", "data", "date", "diff", "disk", "draw", "drop",
    "dump", "echo", "edge", "edit", "else", "enum", "exit", "file", "find", "flag",
    "flow", "free", "glob", "grid", "hash", "head", "heap", "host", "icon", "info",
    "init", "item", "join", "json", "leaf", "line", "link", "list", "load", "lock",
    "loop", "main", "mesh", "mock", "mode", "node", "null", "open", "path", "pipe",
    "pool", "port", "push", "root", "save",
    // Length 5 (85)
    "abort", "actor", "agent", "alert", "alias", "align", "alloc", "apply", "array", "asset",
    "async", "audio", "await", "batch", "bench", "blank", "block", "board", "bound", "break",
    "build", "cache", "catch", "chain", "check", "chunk", "clamp", "class", "clean", "clear",
    "clone", "close", "cloud", "codec", "color", "const", "count", "crash", "debug", "delay",
    "dirty", "draft", "drain", "drive", "entry", "epoch", "error", "event", "exact", "false",
    "fault", "fetch", "field", "float", "floor", "flush", "focus", "force", "frame", "graph",
    "group", "guard", "image", "index", "input", "layer", "limit", "macro", "match", "model",
    "mutex", "order", "panel", "param", "parse", "patch", "pause", "pixel", "point", "print",
    "query", "queue", "reset", "scope", "stack",
    // Length 6 (95)
    "action", "active", "anchor", "assert", "atomic", "binary", "bitmap", "branch", "bridge", "buffer",
    "bundle", "button", "camera", "canvas", "change", "client", "commit", "config", "create", "crypto",
    "cursor", "daemon", "damage", "decode", "delete", "deploy", "derive", "device", "dialog", "digest",
    "direct", "domain", "driver", "encode", "engine", "entity", "escape", "export", "extent", "filter",
    "finish", "folder", "format", "freeze", "gadget", "global", "handle", "header", "helper", "import",
    "indent", "inline", "insert", "invoke", "layout", "linear", "listen", "loader", "lookup", "matrix",
    "member", "memory", "method", "module", "native", "number", "object", "offset", "opaque", "output",
    "packet", "parser", "plugin", "policy", "prefix", "profil", "random", "record", "refact", "region",
    "render", "result", "resume", "return", "router", "scalar", "schema", "screen", "script", "search",
    "sender", "sensor", "serial", "server", "signal",
    // Length 7 (85)
    "adapter", "address", "advance", "archive", "backend", "barrier", "bitmask", "boolean", "browser", "builder",
    "capture", "channel", "cleanup", "closure", "cluster", "command", "compile", "complex", "compute", "connect",
    "console", "context", "control", "convert", "counter", "current", "dataset", "decoder", "default", "desktop",
    "destroy", "disable", "discard", "display", "dynamic", "element", "encoder", "execute", "factory", "feature",
    "forward", "gateway", "generic", "handler", "include", "indexer", "inspect", "integer", "iterate", "keyword",
    "library", "license", "manager", "mapping", "message", "monitor", "network", "numeric", "package", "payload",
    "pipeline", "pointer", "polling", "preview", "process", "profile", "project", "protect", "receipt", "release",
    "request", "resolve", "restart", "routine", "runtime", "sampler", "segment", "service", "session", "storage",
    "surface", "tracker", "trigger", "utility", "version",
    // Length 8 (80)
    "abstract", "accessor", "activate", "allocate", "argument", "assemble", "baseline", "boundary", "bytecode", "callback",
    "checksum", "compiler", "compress", "constant", "consumer", "database", "datagram", "debugger", "decryption", "delegate",
    "detector", "dispatch", "document", "download", "endpoint", "estimate", "evaluate", "exchange", "executor", "fallback",
    "feedback", "frontend", "function", "generate", "geometry", "hardware", "identity", "instance", "integral", "internal",
    "interval", "iterator", "lifetime", "listener", "loopback", "manifest", "metadata", "minimize", "modifier", "mutation",
    "observer", "operator", "optimize", "overflow", "overhead", "parallel", "pipeline", "platform", "position", "priority",
    "producer", "property", "protocol", "provider", "receiver", "recovery", "redirect", "register", "renderer", "resource",
    "response", "rollback", "sanitize", "schedule", "selector", "sequence", "snapshot", "software", "template", "terminal",
    // Length 9 (60)
    "aggregate", "algorithm", "allocator", "animation", "anonymous", "assertion", "attribute", "benchmark", "broadcast", "candidate",
    "collision", "component", "condition", "configure", "connector", "container", "dashboard", "dataframe", "decorator", "delimiter",
    "directory", "duplicate", "exception", "execution", "framework", "generator", "heuristic", "hierarchy", "immutable", "implement",
    "increment", "indicator", "infection", "injection", "inspector", "interface", "invariant", "iteration", "migration", "namespace",
    "operation", "optimizer", "parameter", "partition", "precision", "profiling", "prototype", "publisher", "reference", "rendering",
    "scheduler", "separator", "singleton", "statement", "structure", "subroutine", "telemetry", "transform", "validator", "viewmodel",
    // Length 10 (40)
    "allocation", "arithmetic", "assembler", "assignment", "attachment", "automation", "background", "bitmapping", "bottleneck", "breakpoint",
    "checkpoint", "clustering", "collection", "comparison", "complexity", "connection", "constraint", "controller", "conversion", "definition",
    "dependency", "deployment", "deprecated", "descriptor", "dictionary", "disconnect", "dispatcher", "encryption", "executable", "filesystem",
    "formatting", "generics", "identifier", "invocation", "javascript", "management", "middleware", "pagination", "permission", "repository",
    // Length 11 (20)
    "aggregation", "application", "calculation", "certificate", "combination",
    "compilation", "compression", "concurrency", "conditional", "constructor",
    "declaration", "destination", "destruction", "environment", "flexibility",
    "inheritance", "instruction", "integration", "interaction", "performance",
    // Length 12+ (20)
    "architecture", "asynchronous", "authentication", "availability", "compatibility",
    "configuration", "cryptography", "customization", "documentation", "encapsulation",
    "functionality", "initialization", "instantaneous", "interceptor", "interoperable",
    "maintainability", "multithreading", "normalization", "orchestration", "parallelization"
};

// -----------------------------------------------------------------------------
// Deterministic 600 Test-Case Generator (12 buckets x 50 test cases)
// -----------------------------------------------------------------------------
vector<TestCase> build600TestSuite() {
    vector<TestCase> suite;
    suite.reserve(600);
    int currentId = 1;

    vector<string> lenGte4, lenGte5, lenGte6, lenGte7, lenGte8;
    for (const auto& w : dictionary) {
        if (w.length() >= 4) lenGte4.push_back(w);
        if (w.length() >= 5) lenGte5.push_back(w);
        if (w.length() >= 6) lenGte6.push_back(w);
        if (w.length() >= 7) lenGte7.push_back(w);
        if (w.length() >= 8) lenGte8.push_back(w);
    }

    // 1. Missing 1 Letter (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte4[i % lenGte4.size()];
        int cutIdx = (i % (gt.length() - 2)) + 1;
        string q = gt.substr(0, cutIdx) + gt.substr(cutIdx + 1);
        suite.push_back({currentId++, q, gt, "Missing 1 Letter", "Omitted letter at index " + to_string(cutIdx)});
    }

    // 2. Missing 2 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte5[i % lenGte5.size()];
        int cutIdx = (i % (gt.length() - 3)) + 1;
        string q = gt.substr(0, cutIdx) + gt.substr(cutIdx + 2);
        suite.push_back({currentId++, q, gt, "Missing 2 Letters", "Omitted 2 contiguous letters"});
    }

    // 3. Missing 3 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte6[i % lenGte6.size()];
        int cutIdx = (i % (gt.length() - 4)) + 1;
        string q = gt.substr(0, cutIdx) + gt.substr(cutIdx + 3);
        suite.push_back({currentId++, q, gt, "Missing 3 Letters", "Omitted 3 contiguous letters"});
    }

    // 4. Extra 1 Letter (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte4[i % lenGte4.size()];
        int insIdx = (i % (gt.length() - 1)) + 1;
        char dup = gt[insIdx];
        string q = gt.substr(0, insIdx) + dup + gt.substr(insIdx);
        suite.push_back({currentId++, q, gt, "Extra 1 Letter", "Duplicated letter at index " + to_string(insIdx)});
    }

    // 5. Extra 2 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte4[i % lenGte4.size()];
        int insIdx = (i % (gt.length() - 1)) + 1;
        string q = gt.substr(0, insIdx) + gt[insIdx] + gt[insIdx] + gt.substr(insIdx);
        suite.push_back({currentId++, q, gt, "Extra 2 Letters", "Double redundant key at index " + to_string(insIdx)});
    }

    // 6. Extra 3 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte4[i % lenGte4.size()];
        int insIdx = (i % (gt.length() - 1)) + 1;
        string q = gt.substr(0, insIdx) + gt[insIdx] + gt[insIdx] + gt[insIdx] + gt.substr(insIdx);
        suite.push_back({currentId++, q, gt, "Extra 3 Letters", "Triple redundant key at index " + to_string(insIdx)});
    }

    // 7. Swap 2 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte4[i % lenGte4.size()];
        string q = gt;
        int idx = i % (gt.length() - 1);
        swap(q[idx], q[idx + 1]);
        suite.push_back({currentId++, q, gt, "Swap 2 Letters", "Adjacent swap (" + to_string(idx) + ", " + to_string(idx + 1) + ")"});
    }

    // 8. Swap 3 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte5[i % lenGte5.size()];
        string q = gt;
        int idx = i % (gt.length() - 2);
        char tmp = q[idx];
        q[idx] = q[idx + 1];
        q[idx + 1] = q[idx + 2];
        q[idx + 2] = tmp;
        suite.push_back({currentId++, q, gt, "Swap 3 Letters", "Cyclic permutation of 3 characters"});
    }

    // 9. Swap 4 Letters (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte6[i % lenGte6.size()];
        string q = gt;
        int idx = i % (gt.length() - 3);
        reverse(q.begin() + idx, q.begin() + idx + 4);
        suite.push_back({currentId++, q, gt, "Swap 4 Letters", "Reversal of 4-character window"});
    }

    // 10. Length Delta 1 (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte4[i % lenGte4.size()];
        string q = (i % 2 == 0) ? (gt + "x") : (gt.substr(0, gt.length() - 1));
        suite.push_back({currentId++, q, gt, "Length Delta 1", "Absolute length gap = 1"});
    }

    // 11. Length Delta 2 (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte5[i % lenGte5.size()];
        string q = (i % 2 == 0) ? (gt + "xy") : (gt.substr(0, gt.length() - 2));
        suite.push_back({currentId++, q, gt, "Length Delta 2", "Absolute length gap = 2"});
    }

    // 12. Length Delta 3 (50)
    for (int i = 0; i < 50; ++i) {
        string gt = lenGte6[i % lenGte6.size()];
        string q = (i % 2 == 0) ? (gt + "xyz") : (gt.substr(0, gt.length() - 3));
        suite.push_back({currentId++, q, gt, "Length Delta 3", "Absolute length gap = 3"});
    }

    return suite;
}

int main() {
    vector<TestCase> testCases = build600TestSuite();

    cout << "========================================================================\n";
    cout << "          SPELLCHECK ENGINE BENCHMARK HARNESS (600 TEST CASES)\n";
    cout << "========================================================================\n";
    cout << "Candidate Lexicon Size : " << dictionary.size() << " developer terms (sorted length-wise)\n";
    cout << "Test Suite Cases       : " << testCases.size() << " cases (12 categories x 50)\n";

    // Export test_cases.txt
    ofstream tcOut(TEST_CASES_FILE);
    tcOut << "=========================================================================================\n";
    tcOut << "                     SPELLCHECK BENCHMARK SUITE (600 TEST CASES)\n";
    tcOut << "=========================================================================================\n";
    tcOut << left << setw(6)  << "ID"
          << setw(22) << "Query"
          << setw(22) << "Ground Truth"
          << setw(24) << "Category"
          << "Transformation Details\n";
    tcOut << string(96, '-') << "\n";

    for (const auto& tc : testCases) {
        tcOut << left << setw(6)  << tc.id
              << setw(22) << tc.query
              << setw(22) << tc.groundTruth
              << setw(24) << tc.category
              << tc.note << "\n";
    }
    tcOut.close();
    cout << "Generated: " << TEST_CASES_FILE << "\n";

    // Run benchmark
    vector<QueryResult> results;
    results.reserve(testCases.size());

    cout << "Executing 600 recommendation passes with dynamic weights...\n";

    for (const auto& tc : testCases) {
        auto tStart = chrono::high_resolution_clock::now();
        vector<Node> topk = recomendation(tc.query, dictionary);
        auto tEnd = chrono::high_resolution_clock::now();

        double lat = chrono::duration<double, micro>(tEnd - tStart).count();

        int matchedRank = 9999;
        float bScore = 1.0f;
        float wLevUsed = 0.50f, wKbdUsed = 0.50f;

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

        if (matchedRank == 9999 && !topk.empty()) {
            bScore = topk[0].score;
        }

        results.push_back({tc, matchedRank, bScore, wLevUsed, wKbdUsed, lat, (matchedRank <= K)});
    }

    // Export test_stats_report.txt
    ofstream statsOut(STATS_REPORT_FILE);
    statsOut << "=========================================================================================\n";
    statsOut << "                 DYNAMIC WEIGHT SPELLCHECK REPORT (600 SAMPLES)\n";
    statsOut << "=========================================================================================\n";
    statsOut << "Total Test Samples Evaluated : " << testCases.size() << "\n";
    statsOut << "Lexicon Reference Pool Size  : " << dictionary.size() << " terms\n";
    statsOut << "Top-K Cutoff                 : " << K << "\n";
    statsOut << "Tuned Hyperparameters        : init=" << init << ", damper=" << damper 
             << ", boost=" << boost << ", lowBnd=" << lowBnd << ", upBnd=" << upBnd << "\n\n";

    statsOut << "-----------------------------------------------------------------------------------------\n";
    statsOut << "PART 1: INDIVIDUAL QUERY TRACE LOG\n";
    statsOut << "-----------------------------------------------------------------------------------------\n";
    statsOut << left << setw(6)  << "ID"
             << setw(20) << "Query"
             << setw(20) << "Ground Truth"
             << setw(8)  << "Rank"
             << setw(12) << "Score"
             << setw(8)  << "W_lev"
             << setw(8)  << "W_kbd"
             << setw(14) << "Latency (us)"
             << "Status\n";
    statsOut << string(94, '-') << "\n";

    for (const auto& r : results) {
        string rankStr = (r.rank <= K) ? ("#" + to_string(r.rank)) : "FAIL";
        string statusStr = (r.rank == 1) ? "TOP-1" : ((r.rank <= K) ? "TOP-K" : "MISS");

        statsOut << left << setw(6)  << r.tc.id
                 << setw(20) << r.tc.query
                 << setw(20) << r.tc.groundTruth
                 << setw(8)  << rankStr
                 << fixed << setprecision(4) << setw(12) << r.bestScore
                 << setprecision(2) << setw(8) << r.wLev
                 << setw(8) << r.wKbd
                 << setprecision(1) << setw(14) << r.latencyUs
                 << statusStr << "\n";
    }

    statsOut << "\n-----------------------------------------------------------------------------------------\n";
    statsOut << "PART 2: CATEGORY-WISE BREAKDOWN (12 BUCKETS x 50 CASES)\n";
    statsOut << "-----------------------------------------------------------------------------------------\n";
    statsOut << left << setw(24) << "Category"
             << setw(9)  << "Samples"
             << setw(12) << "Top-1 Acc"
             << setw(12) << "Top-5 Acc"
             << setw(10) << "MRR"
             << setw(10) << "Avg W_lev"
             << setw(10) << "Avg W_kbd"
             << "Avg Lat (us)\n";
    statsOut << string(96, '-') << "\n";

    vector<string> categoryNames = {
        "Missing 1 Letter", "Missing 2 Letters", "Missing 3 Letters",
        "Extra 1 Letter", "Extra 2 Letters", "Extra 3 Letters",
        "Swap 2 Letters", "Swap 3 Letters", "Swap 4 Letters",
        "Length Delta 1", "Length Delta 2", "Length Delta 3"
    };

    int globalTop1 = 0, globalTop5 = 0;
    double globalMrrSum = 0.0, globalLatSum = 0.0, globalWlevSum = 0.0, globalWkbdSum = 0.0;

    for (const auto& cat : categoryNames) {
        int count = 0, top1 = 0, top5 = 0;
        double mrr = 0.0, sumWlev = 0.0, sumWkbd = 0.0, sumLat = 0.0;

        for (const auto& r : results) {
            if (r.tc.category == cat) {
                count++;
                if (r.rank == 1) top1++;
                if (r.rank <= K) top5++;
                if (r.rank <= K) mrr += (1.0 / static_cast<double>(r.rank));
                sumWlev += r.wLev;
                sumWkbd += r.wKbd;
                sumLat += r.latencyUs;
            }
        }

        double catTop1Acc = (count > 0) ? (static_cast<double>(top1) / count * 100.0) : 0.0;
        double catTop5Acc = (count > 0) ? (static_cast<double>(top5) / count * 100.0) : 0.0;
        double catMrr     = (count > 0) ? (mrr / count) : 0.0;
        double avgWlev    = (count > 0) ? (sumWlev / count) : 0.0;
        double avgWkbd    = (count > 0) ? (sumWkbd / count) : 0.0;
        double avgLatency = (count > 0) ? (sumLat / count) : 0.0;

        globalTop1    += top1;
        globalTop5    += top5;
        globalMrrSum  += mrr;
        globalLatSum  += sumLat;
        globalWlevSum += sumWlev;
        globalWkbdSum += sumWkbd;

        statsOut << left << setw(24) << cat
                 << setw(9)  << count
                 << fixed << setprecision(1) << setw(12) << (to_string((int)catTop1Acc) + "%")
                 << setw(12) << (to_string((int)catTop5Acc) + "%")
                 << setprecision(3) << setw(10) << catMrr
                 << setprecision(2) << setw(10) << avgWlev
                 << setw(10) << avgWkbd
                 << setprecision(1) << avgLatency << "\n";
    }

    double totalCount = static_cast<double>(results.size());
    double finalTop1Acc    = (totalCount > 0) ? (globalTop1 / totalCount * 100.0) : 0.0;
    double finalTop5Acc    = (totalCount > 0) ? (globalTop5 / totalCount * 100.0) : 0.0;
    double finalMrr        = (totalCount > 0) ? (globalMrrSum / totalCount) : 0.0;
    double finalAvgLatency = (totalCount > 0) ? (globalLatSum / totalCount) : 0.0;
    double finalAvgWlev    = (totalCount > 0) ? (globalWlevSum / totalCount) : 0.0;
    double finalAvgWkbd    = (totalCount > 0) ? (globalWkbdSum / totalCount) : 0.0;

    statsOut << "\n-----------------------------------------------------------------------------------------\n";
    statsOut << "PART 3: GLOBAL AGGREGATE SUMMARY\n";
    statsOut << "-----------------------------------------------------------------------------------------\n";
    statsOut << "Total Queries Evaluated         : " << results.size() << "\n";
    statsOut << "Top-1 Accuracy                 : " << fixed << setprecision(2) << finalTop1Acc << "%\n";
    statsOut << "Top-5 Accuracy                 : " << fixed << setprecision(2) << finalTop5Acc << "%\n";
    statsOut << "Mean Reciprocal Rank (MRR)      : " << setprecision(4) << finalMrr << "\n";
    statsOut << "Average Query Latency           : " << setprecision(2) << finalAvgLatency << " microseconds\n";
    statsOut << "Global Dynamic Weight Balance   : W_lev = " << setprecision(2) << finalAvgWlev 
             << " | W_kbd = " << finalAvgWkbd << "\n";
    statsOut << "=========================================================================================\n";
    statsOut.close();

    cout << "Benchmark complete!\n";
    cout << "Global Top-1 Accuracy : " << finalTop1Acc << "%\n";
    cout << "Global Top-5 Accuracy : " << finalTop5Acc << "%\n";
    cout << "Mean Reciprocal Rank  : " << finalMrr << "\n";
    cout << "Generated files:\n  - " << TEST_CASES_FILE << "\n  - " << STATS_REPORT_FILE << "\n";

    return 0;
}
