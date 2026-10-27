#include <iostream>
#include <string>
#include <vector>
#include <chrono>

using namespace std;

#define K 3
#define MAX_WORD_LEN 25
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


vector<Node> recomendation(string& incorrectWord, vector<string>& list) {

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
        // float totalScore = Score; // Using only Levenshtein distance for scoring////////////////////////////////////////

        tracker.insert(totalScore, candidate);
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

    while(1){
        string incWrd = ""; // Example incorrect word

        cout << "\nEnter an incorrect word : ";
        cin >> incWrd;

        
        auto start = std::chrono::high_resolution_clock::now();

        vector<Node> topk = recomendation(incWrd, list);

        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> elapsed = end - start;
        cout << "Elapsed time: " << elapsed.count() << " ms\n";

        cout << "Top " << topk.size() << " recommendations for \"" << incWrd << "\":\n";
        for (size_t i = 0; i < topk.size(); ++i) {
            cout << i + 1 << ". " << topk[i].word << " ( Score: " << topk[i].score << ")\n";
        }
    }

    return 0;
}