#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cctype>

using namespace std;

#define K 3
#define MAX_WORD_LEN 25
constexpr float upBnd   = 0.7f;
constexpr float lowBnd  = 0.05f;
constexpr float damper  = 0.035f;
constexpr float boost   = 0.11f;
constexpr float init    = 0.5f;

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

// Standard staggered QWERTY coordinates
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
    a = static_cast<char>(tolower(a));
    b = static_cast<char>(tolower(b));
    if (a < 'a' || a > 'z' || b < 'a' || b > 'z') return 1.0f;

    const XY& p1 = KB_MAP[a - 'a'];
    const XY& p2 = KB_MAP[b - 'a'];

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dist = sqrt(dx * dx + dy * dy);

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

// Primary recommendation pipeline integrating dynamic weight derivation with composite scoring
vector<Node> recomendation(const string& incorrectWord, const vector<string>& list) {
    TopKTracker tracker;

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

    return tracker.getSortedResults();
}

int main() {
    vector<string> list = {
    // Length 2 (10 words)
    "as", "at", "by", "do", "go", "if", "in", "is", "it", "to",
    // Length 3 (25 words)
    "and", "app", "bit", "bug", "col", "cur", "doc", "end", "fit", "for",
    "get", "git", "int", "key", "log", "map", "net", "new", "ptr", "raw",
    "row", "run", "set", "try", "val",
    // Length 4 (40 words)
    "args", "base", "bind", "byte", "call", "case", "char", "code", "copy", "data",
    "date", "draw", "drop", "edit", "else", "enum", "file", "find", "free", "hash",
    "heap", "init", "item", "link", "list", "load", "loop", "main", "mock", "mode",
    "null", "open", "path", "push", "root", "save", "size", "sort", "task", "tree",
    // Length 5 (50 words)
    "abort", "alert", "alias", "align", "alloc", "apply", "array", "async", "await", "block",
    "board", "bound", "break", "build", "catch", "check", "class", "clean", "clear", "clone",
    "close", "count", "debug", "delay", "dirty", "draft", "drive", "event", "exact", "false",
    "fetch", "field", "float", "floor", "flush", "focus", "frame", "index", "input", "match",
    "model", "mutex", "parse", "print", "query", "queue", "reset", "scope", "stack", "state",
    // Length 6 (50 words)
    "action", "active", "binary", "branch", "buffer", "bundle", "button", "canvas", "change", "client",
    "config", "create", "cursor", "delete", "deploy", "derive", "direct", "domain", "engine", "entity",
    "export", "filter", "finish", "format", "global", "handle", "header", "helper", "import", "insert",
    "invoke", "layout", "listen", "memory", "method", "native", "object", "output", "packet", "parser",
    "plugin", "prefix", "record", "render", "result", "return", "schema", "script", "search", "server",
    // Length 7 (45 words)
    "adapter", "advance", "archive", "boolean", "builder", "capture", "channel", "command", "compile", "connect",
    "context", "control", "convert", "current", "default", "disable", "display", "dynamic", "element", "execute",
    "factory", "feature", "generic", "handler", "include", "iterate", "keyword", "library", "license", "manager",
    "message", "network", "package", "pointer", "process", "profile", "release", "request", "resolve", "runtime",
    "service", "session", "storage", "tracker", "trigger",
    // Length 8 (35 words)
    "abstract", "allocate", "argument", "callback", "compiler", "database", "debugger", "delegate", "document", "download",
    "endpoint", "evaluate", "executor", "function", "generate", "instance", "internal", "iterator", "metadata", "observer",
    "operator", "optimize", "pipeline", "platform", "priority", "property", "protocol", "receiver", "register", "response",
    "schedule", "selector", "sequence", "template", "terminal",
    // Length 9 (25 words)
    "algorithm", "animation", "anonymous", "assertion", "attribute", "benchmark", "component", "condition", "configure", "connector",
    "container", "decorator", "duplicate", "exception", "execution", "generator", "immutable", "implement", "increment", "indicator",
    "injection", "interface", "parameter", "reference", "structure",
    // Length 10 (10 words)
    "allocation", "breakpoint", "controller", "dependency", "encryption",
    "middleware", "pagination", "repository", "stylesheet", "validation",
    // Length 11+ (10 words)
    "compilation", "concurrency", "environment", "inheritance", "integration",
    "interaction", "performance", "placeholder", "transaction", "asynchronous"
};

    cout << "===============================================================\n";
    cout << "  SPELLCHECK ENGINE (DYNAMIC ADAPTIVE WEIGHTING)\n";
    cout << "===============================================================\n";

    while (true) {
        string incWrd;
        cout << "\nEnter an incorrect word : ";
        cin >> incWrd;

        auto start = chrono::high_resolution_clock::now();

        vector<Node> topk = recomendation(incWrd, list);

        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double, milli> elapsed = end - start;
        cout << "Execution time: " << fixed << setprecision(3) << elapsed.count() << " ms\n";

        cout << "Top " << topk.size() << " recommendations for \"" << incWrd << "\":\n";
        cout << left << setw(4) << "#" 
             << setw(16) << "Candidate" 
             << setw(14) << "Total Score" 
             << setw(10) << "W_lev" 
             << setw(10) << "W_kbd" << "\n";
        cout << string(54, '-') << "\n";

        for (size_t i = 0; i < topk.size(); ++i) {
            cout << left << setw(4) << (to_string(i + 1) + ".")
                 << setw(16) << topk[i].word
                 << fixed << setprecision(4) << setw(14) << topk[i].score
                 << setprecision(2) << setw(10) << topk[i].wLev
                 << setw(10) << topk[i].wKbd << "\n";
        }
    }

    return 0;
}