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

#define K 3
#define MAX_WORD_LEN 32

// =============================================================================
// Configurable Parameter Definitions
// =============================================================================
constexpr float upBnd   = 0.7f;
constexpr float lowBnd  = 0.05f;
constexpr float damper  = 0.035f;
constexpr float boost   = 0.11f;
constexpr float init    = 0.5f;

const std::string outputFileName = "output6.txt";

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
    string notes;
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
        outWlev = 0.70f;
        outWkbd = 0.30f;
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
        if (tolower(s1[i]) != tolower(s2[i])) {
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

int main() {
    vector<string> list = {
        "cake", "calm", "came", "camp", "card", "care", "case", "cash", "cave", "cell", "cent",
        "chat", "chef", "cheek", "cheer", "cheese", "cheque", "chess", "chest", "chief", "child",
        "chill", "china", "choice", "choose", "church", "circle", "circuit", "citizen", "classic",
        "climate", "clock", "closet", "coffee", "collar", "college", "colour", "comfort", "command",
        "computer", "complete", "consider", "construction", "the", "and", "cat", "bat", "rat", 
        "hat", "fat", "mat", "cut", "dog", "fog", "log", "sun", "fun", "run", "bun", "can", "cap", 
        "cab", "bed", "pen", "pin", "pat", "book", "back", "bark", "dark", "park", "word", "work", 
        "walk", "wall", "ball", "call", "fall", "tall", "hand", "band", "land", "sand", "home", 
        "come", "some", "time", "line", "life", "live", "like", "look", "took", "food", "good", 
        "wood", "mood", "fast", "last", "past", "cast", "cost", "post", "most", "host", "help", 
        "held", "hold", "gold", "cold", "told", "friend", "family", "school", "street", "window", 
        "yellow", "orange", "purple", "market", "garden", "player", "action", "nation", "person", 
        "people", "system", "method", "vector", "string", "simple", "sample", "middle", "bottle", 
        "little", "settle", "handle", "candle", "bundle", "butter", "better", "bitter", "letter", 
        "winter", "summer", "spring", "testing", "running", "walking", "talking", "reading", 
        "writing", "playing", "feeling", "looking", "working", "holding", "helping", "opening", 
        "closing", "movement", "judgment", "payment", "shipment", "agreement", "treatment", 
        "station", "section", "mention", "fraction", "reaction", "position", "condition", "relation", 
        "solution", "decision", "provision", "revision", "important", "different", "difficult", 
        "necessary", "president", "government", "development", "environment", "information", 
        "organization", "communication", "transportation", "understanding", "programming", 
        "performance", "algorithm", "similarity", "coefficient", "candidate", "correction", 
        "keyboard", "distance", "caterpillar", "administration", "professor", "doctor", "lawyer",
        "police", "officer", "artist", "island", "nature", "forest", "mountain", "river", "bridge"
    };

    vector<TestCase> testCases = {
        // [Category 1: Adjacent Horizontal Slips] (50 cases)
        {1, "cst", "cat", "Adjacent Horizontal Slips", "s -> a"},
        {2, "vome", "come", "Adjacent Horizontal Slips", "v -> c"},
        {3, "gome", "home", "Adjacent Horizontal Slips", "g -> h"},
        {4, "pwn", "pen", "Adjacent Horizontal Slips", "w -> e"},
        {5, "doh", "dog", "Adjacent Horizontal Slips", "h -> g"},
        {6, "bex", "bed", "Adjacent Horizontal Slips", "x -> d"},
        {7, "woek", "work", "Adjacent Horizontal Slips", "e -> r"},
        {8, "dakk", "dark", "Adjacent Horizontal Slips", "k -> r"},
        {9, "wsll", "wall", "Adjacent Horizontal Slips", "s -> a"},
        {10, "gsst", "fast", "Adjacent Horizontal Slips", "g -> f"},
        {11, "mowt", "most", "Adjacent Horizontal Slips", "w -> s"},
        {12, "hols", "hold", "Adjacent Horizontal Slips", "s -> d"},
        {13, "cild", "cold", "Adjacent Horizontal Slips", "i -> o"},
        {14, "griend", "friend", "Adjacent Horizontal Slips", "g -> f"},
        {15, "syetem", "system", "Adjacent Horizontal Slips", "e -> s"},
        {16, "methof", "method", "Adjacent Horizontal Slips", "f -> d"},
        {17, "vecyor", "vector", "Adjacent Horizontal Slips", "y -> t"},
        {18, "semple", "sample", "Adjacent Horizontal Slips", "e -> a"},
        {19, "bitzer", "bitter", "Adjacent Horizontal Slips", "z -> t"},
        {20, "spfing", "spring", "Adjacent Horizontal Slips", "f -> r"},
        {21, "natute", "nature", "Adjacent Horizontal Slips", "t -> r"},
        {22, "marl", "mark", "Adjacent Horizontal Slips", "l -> k"},
        {23, "caln", "calm", "Adjacent Horizontal Slips", "n -> m"},
        {24, "caze", "care", "Adjacent Horizontal Slips", "z -> a"},
        {25, "cask", "cash", "Adjacent Horizontal Slips", "k -> h"},
        {26, "vave", "cave", "Adjacent Horizontal Slips", "v -> c"},
        {27, "xell", "cell", "Adjacent Horizontal Slips", "x -> c"},
        {28, "vrnt", "cent", "Adjacent Horizontal Slips", "r -> e"},
        {29, "vhat", "chat", "Adjacent Horizontal Slips", "v -> c"},
        {30, "chrf", "chef", "Adjacent Horizontal Slips", "r -> e"},
        {31, "chewk", "cheek", "Adjacent Horizontal Slips", "w -> e"},
        {32, "chrrr", "cheer", "Adjacent Horizontal Slips", "r -> e"},
        {33, "chrs", "chess", "Adjacent Horizontal Slips", "r -> e"},
        {34, "chist", "chest", "Adjacent Horizontal Slips", "i -> e"},
        {35, "chird", "child", "Adjacent Horizontal Slips", "r -> l"},
        {36, "chilll", "chill", "Adjacent Horizontal Slips", "redundant l"},
        {37, "chona", "china", "Adjacent Horizontal Slips", "o -> i"},
        {38, "choise", "choice", "Adjacent Horizontal Slips", "s -> c"},
        {39, "vhurch", "church", "Adjacent Horizontal Slips", "v -> c"},
        {40, "citizrn", "citizen", "Adjacent Horizontal Slips", "r -> e"},
        {41, "cloxk", "clock", "Adjacent Horizontal Slips", "x -> c"},
        {42, "closte", "closet", "Adjacent Horizontal Slips", "t -> e"},
        {43, "coffe", "coffee", "Adjacent Horizontal Slips", "missing e"},
        {44, "collst", "collar", "Adjacent Horizontal Slips", "s -> a"},
        {45, "colleqe", "college", "Adjacent Horizontal Slips", "q -> g"},
        {46, "comfirt", "comfort", "Adjacent Horizontal Slips", "i -> o"},
        {47, "commamd", "command", "Adjacent Horizontal Slips", "m -> n"},
        {48, "computrr", "computer", "Adjacent Horizontal Slips", "r -> e"},
        {49, "considrr", "consider", "Adjacent Horizontal Slips", "r -> e"},
        {50, "hellp", "help", "Adjacent Horizontal Slips", "p -> o proximity"},

        // [Category 2: Adjacent Vertical / Diagonal Slips] (50 cases)
        {51, "xat", "cat", "Adjacent Vertical/Diagonal Slips", "x -> c"},
        {52, "wnd", "and", "Adjacent Vertical/Diagonal Slips", "w -> a"},
        {53, "eun", "run", "Adjacent Vertical/Diagonal Slips", "e -> r"},
        {54, "dof", "dog", "Adjacent Vertical/Diagonal Slips", "f -> g"},
        {55, "csm", "cam", "Adjacent Vertical/Diagonal Slips", "s -> a"},
        {56, "sare", "care", "Adjacent Vertical/Diagonal Slips", "s -> c"},
        {57, "kife", "life", "Adjacent Vertical/Diagonal Slips", "k -> l"},
        {58, "fook", "took", "Adjacent Vertical/Diagonal Slips", "f -> t"},
        {59, "hsnd", "hand", "Adjacent Vertical/Diagonal Slips", "s -> a"},
        {60, "gond", "good", "Adjacent Vertical/Diagonal Slips", "n -> o"},
        {61, "peopke", "people", "Adjacent Vertical/Diagonal Slips", "k -> l"},
        {62, "botrle", "bottle", "Adjacent Vertical/Diagonal Slips", "r -> t"},
        {63, "winrer", "winter", "Adjacent Vertical/Diagonal Slips", "r -> t"},
        {64, "docror", "doctor", "Adjacent Vertical/Diagonal Slips", "r -> t"},
        {65, "lawywr", "lawyer", "Adjacent Vertical/Diagonal Slips", "w -> e"},
        {66, "arrist", "artist", "Adjacent Vertical/Diagonal Slips", "r -> t"},
        {67, "solder", "soldier", "Adjacent Vertical/Diagonal Slips", "i omission"},
        {68, "auyhor", "author", "Adjacent Vertical/Diagonal Slips", "y -> t"},
        {69, "reacyion", "reaction", "Adjacent Vertical/Diagonal Slips", "y -> t"},
        {70, "xake", "cake", "Adjacent Vertical/Diagonal Slips", "x -> c"},
        {71, "vsrd", "card", "Adjacent Vertical/Diagonal Slips", "s -> a"},
        {72, "csse", "case", "Adjacent Vertical/Diagonal Slips", "s -> a"},
        {73, "dave", "cave", "Adjacent Vertical/Diagonal Slips", "d -> c"},
        {74, "drll", "cell", "Adjacent Vertical/Diagonal Slips", "r -> e"},
        {75, "xent", "cent", "Adjacent Vertical/Diagonal Slips", "x -> c"},
        {76, "xhat", "chat", "Adjacent Vertical/Diagonal Slips", "x -> c"},
        {77, "dhef", "chef", "Adjacent Vertical/Diagonal Slips", "d -> c"},
        {78, "chrrk", "cheek", "Adjacent Vertical/Diagonal Slips", "r -> e"},
        {79, "chrrr", "cheer", "Adjacent Vertical/Diagonal Slips", "r -> e"},
        {80, "vheque", "cheque", "Adjacent Vertical/Diagonal Slips", "v -> c"},
        {81, "chrss", "chess", "Adjacent Vertical/Diagonal Slips", "r -> e"},
        {82, "chyst", "chest", "Adjacent Vertical/Diagonal Slips", "y -> e"},
        {83, "chuef", "chief", "Adjacent Vertical/Diagonal Slips", "u -> i"},
        {84, "chuld", "child", "Adjacent Vertical/Diagonal Slips", "u -> i"},
        {85, "chull", "chill", "Adjacent Vertical/Diagonal Slips", "u -> i"},
        {86, "chuna", "china", "Adjacent Vertical/Diagonal Slips", "u -> i"},
        {87, "chooce", "choice", "Adjacent Vertical/Diagonal Slips", "o -> i"},
        {88, "vhoose", "choose", "Adjacent Vertical/Diagonal Slips", "v -> c"},
        {89, "chutch", "church", "Adjacent Vertical/Diagonal Slips", "t -> r"},
        {90, "cirvle", "circle", "Adjacent Vertical/Diagonal Slips", "v -> c"},
        {91, "cirsuit", "circuit", "Adjacent Vertical/Diagonal Slips", "s -> c"},
        {92, "clsssic", "classic", "Adjacent Vertical/Diagonal Slips", "s -> a"},
        {93, "vlimste", "climate", "Adjacent Vertical/Diagonal Slips", "v -> c"},
        {94, "vlok", "clock", "Adjacent Vertical/Diagonal Slips", "v -> c"},
        {95, "closrt", "closet", "Adjacent Vertical/Diagonal Slips", "r -> e"},
        {96, "collsr", "collar", "Adjacent Vertical/Diagonal Slips", "s -> a"},
        {97, "xollege", "college", "Adjacent Vertical/Diagonal Slips", "x -> c"},
        {98, "colout", "colour", "Adjacent Vertical/Diagonal Slips", "t -> r"},
        {99, "comfot", "comfort", "Adjacent Vertical/Diagonal Slips", "omission r"},
        {100, "conmand", "command", "Adjacent Vertical/Diagonal Slips", "n -> m"},

        // [Category 3: Single Deletions / Omissions] (50 cases)
        {101, "playng", "playing", "Single Deletions", "omitted i"},
        {102, "readng", "reading", "Single Deletions", "omitted i"},
        {103, "walkng", "walking", "Single Deletions", "omitted i"},
        {104, "talkng", "talking", "Single Deletions", "omitted i"},
        {105, "feelng", "feeling", "Single Deletions", "omitted i"},
        {106, "openng", "opening", "Single Deletions", "omitted i"},
        {107, "famly", "family", "Single Deletions", "omitted i"},
        {108, "gardn", "garden", "Single Deletions", "omitted e"},
        {109, "systm", "system", "Single Deletions", "omitted e"},
        {110, "persn", "person", "Single Deletions", "omitted o"},
        {111, "simpl", "simple", "Single Deletions", "omitted e"},
        {112, "settl", "settle", "Single Deletions", "omitted e"},
        {113, "buttr", "butter", "Single Deletions", "omitted e"},
        {114, "forst", "forest", "Single Deletions", "omitted e"},
        {115, "rivr", "river", "Single Deletions", "omitted e"},
        {116, "bridg", "bridge", "Single Deletions", "omitted e"},
        {117, "doctr", "doctor", "Single Deletions", "omitted o"},
        {118, "swimmng", "swimming", "Single Deletions", "omitted i"},
        {119, "planng", "planning", "Single Deletions", "omitted i"},
        {120, "movment", "movement", "Single Deletions", "omitted e"},
        {121, "judgmet", "judgment", "Single Deletions", "omitted n"},
        {122, "staton", "station", "Single Deletions", "omitted i"},
        {123, "secton", "section", "Single Deletions", "omitted i"},
        {124, "positon", "position", "Single Deletions", "omitted i"},
        {125, "acton", "action", "Single Deletions", "omitted i"},
        {126, "naton", "nation", "Single Deletions", "omitted i"},
        {127, "soluton", "solution", "Single Deletions", "omitted i"},
        {128, "decison", "decision", "Single Deletions", "omitted i"},
        {129, "menton", "mention", "Single Deletions", "omitted i"},
        {130, "fracton", "fraction", "Single Deletions", "omitted i"},
        {131, "reacton", "reaction", "Single Deletions", "omitted i"},
        {132, "conditon", "condition", "Single Deletions", "omitted i"},
        {133, "relaton", "relation", "Single Deletions", "omitted i"},
        {134, "provisn", "provision", "Single Deletions", "omitted io"},
        {135, "revisn", "revision", "Single Deletions", "omitted io"},
        {136, "closng", "closing", "Single Deletions", "omitted i"},
        {137, "holdng", "holding", "Single Deletions", "omitted i"},
        {138, "helpng", "helping", "Single Deletions", "omitted i"},
        {139, "lookng", "looking", "Single Deletions", "omitted i"},
        {140, "workng", "working", "Single Deletions", "omitted i"},
        {141, "runnng", "running", "Single Deletions", "omitted i"},
        {142, "writng", "writing", "Single Deletions", "omitted i"},
        {143, "paymnt", "payment", "Single Deletions", "omitted e"},
        {144, "shipmnt", "shipment", "Single Deletions", "omitted e"},
        {145, "agreemnt", "agreement", "Single Deletions", "omitted e"},
        {146, "treatmnt", "treatment", "Single Deletions", "omitted e"},
        {147, "presidnt", "president", "Single Deletions", "omitted e"},
        {148, "govermnt", "government", "Single Deletions", "omitted ne"},
        {149, "developpnt", "development", "Single Deletions", "omitted me"},
        {150, "environmnt", "environment", "Single Deletions", "omitted e"},

        // [Category 4: Single Insertions / Redundancy] (50 cases)
        {151, "commputer", "computer", "Single Insertions", "redundant m"},
        {152, "proggamming", "programming", "Single Insertions", "redundant g"},
        {153, "differrent", "different", "Single Insertions", "redundant r"},
        {154, "difficcult", "difficult", "Single Insertions", "redundant c"},
        {155, "goverrnment", "government", "Single Insertions", "redundant r"},
        {156, "developpment", "development", "Single Insertions", "redundant p"},
        {157, "neccessary", "necessary", "Single Insertions", "redundant c"},
        {158, "enviroonment", "environment", "Single Insertions", "redundant o"},
        {159, "informmation", "information", "Single Insertions", "redundant m"},
        {160, "orgaanization", "organization", "Single Insertions", "redundant a"},
        {161, "communiccation", "communication", "Single Insertions", "redundant c"},
        {162, "perfomance", "performance", "Single Insertions", "redundant key"},
        {163, "canddidate", "candidate", "Single Insertions", "redundant d"},
        {164, "begginning", "beginning", "Single Insertions", "redundant g"},
        {165, "buillding", "building", "Single Insertions", "redundant l"},
        {166, "churcch", "church", "Single Insertions", "redundant c"},
        {167, "ciircle", "circle", "Single Insertions", "redundant i"},
        {168, "coffeer", "coffee", "Single Insertions", "redundant r"},
        {169, "colllege", "college", "Single Insertions", "redundant l"},
        {170, "markket", "market", "Single Insertions", "redundant k"},
        {171, "caake", "cake", "Single Insertions", "redundant a"},
        {172, "caalm", "calm", "Single Insertions", "redundant a"},
        {173, "caame", "came", "Single Insertions", "redundant a"},
        {174, "cammp", "camp", "Single Insertions", "redundant m"},
        {175, "caard", "card", "Single Insertions", "redundant a"},
        {176, "caare", "care", "Single Insertions", "redundant a"},
        {177, "caase", "case", "Single Insertions", "redundant a"},
        {178, "caash", "cash", "Single Insertions", "redundant a"},
        {179, "caave", "cave", "Single Insertions", "redundant a"},
        {180, "ceell", "cell", "Single Insertions", "redundant e"},
        {181, "ceent", "cent", "Single Insertions", "redundant e"},
        {182, "chaat", "chat", "Single Insertions", "redundant a"},
        {183, "cheef", "chef", "Single Insertions", "redundant e"},
        {184, "cheeek", "cheek", "Single Insertions", "redundant e"},
        {185, "cheeer", "cheer", "Single Insertions", "redundant e"},
        {186, "cheeese", "cheese", "Single Insertions", "redundant e"},
        {187, "chesss", "chess", "Single Insertions", "redundant s"},
        {188, "chesst", "chest", "Single Insertions", "redundant s"},
        {189, "chiief", "chief", "Single Insertions", "redundant i"},
        {190, "chiild", "child", "Single Insertions", "redundant i"},
        {191, "chiill", "chill", "Single Insertions", "redundant i"},
        {192, "chiina", "china", "Single Insertions", "redundant i"},
        {193, "chooice", "choice", "Single Insertions", "redundant o"},
        {194, "chooose", "choose", "Single Insertions", "redundant o"},
        {195, "churrch", "church", "Single Insertions", "redundant r"},
        {196, "cirrcle", "circle", "Single Insertions", "redundant r"},
        {197, "circuiit", "circuit", "Single Insertions", "redundant i"},
        {198, "citizenn", "citizen", "Single Insertions", "redundant n"},
        {199, "classsic", "classic", "Single Insertions", "redundant s"},
        {200, "climaate", "climate", "Single Insertions", "redundant a"},

        // [Category 5: Structural Transpositions & Length Scale Imbalances] (50 cases)
        {201, "teh", "the", "Structural & Scale", "transposition e-h"},
        {202, "adn", "and", "Structural & Scale", "transposition d-n"},
        {203, "cta", "cat", "Structural & Scale", "transposition t-a"},
        {204, "bta", "bat", "Structural & Scale", "transposition t-a"},
        {205, "rta", "rat", "Structural & Scale", "transposition t-a"},
        {206, "hta", "hat", "Structural & Scale", "transposition t-a"},
        {207, "fta", "fat", "Structural & Scale", "transposition t-a"},
        {208, "mta", "mat", "Structural & Scale", "transposition t-a"},
        {209, "ctu", "cut", "Structural & Scale", "transposition t-u"},
        {210, "dgo", "dog", "Structural & Scale", "transposition g-o"},
        {211, "fgo", "fog", "Structural & Scale", "transposition g-o"},
        {212, "lgo", "log", "Structural & Scale", "transposition g-o"},
        {213, "snu", "sun", "Structural & Scale", "transposition n-u"},
        {214, "fnu", "fun", "Structural & Scale", "transposition n-u"},
        {215, "rnu", "run", "Structural & Scale", "transposition n-u"},
        {216, "bnu", "bun", "Structural & Scale", "transposition n-u"},
        {217, "cna", "can", "Structural & Scale", "transposition n-a"},
        {218, "cpa", "cap", "Structural & Scale", "transposition p-a"},
        {219, "cba", "cab", "Structural & Scale", "transposition b-a"},
        {220, "bde", "bed", "Structural & Scale", "transposition d-e"},
        {221, "pne", "pen", "Structural & Scale", "transposition n-e"},
        {222, "pni", "pin", "Structural & Scale", "transposition n-i"},
        {223, "pta", "pat", "Structural & Scale", "transposition t-a"},
        {224, "boko", "book", "Structural & Scale", "transposition k-o"},
        {225, "bcka", "back", "Structural & Scale", "transposition k-a"},
        {226, "bakr", "bark", "Structural & Scale", "transposition r-k"},
        {227, "dakr", "dark", "Structural & Scale", "transposition r-k"},
        {228, "pakr", "park", "Structural & Scale", "transposition r-k"},
        {229, "wodr", "word", "Structural & Scale", "transposition d-r"},
        {230, "wokr", "work", "Structural & Scale", "transposition k-r"},
        {231, "wakl", "walk", "Structural & Scale", "transposition k-l"},
        {232, "wlal", "wall", "Structural & Scale", "transposition l-a"},
        {233, "blal", "ball", "Structural & Scale", "transposition l-a"},
        {234, "clal", "call", "Structural & Scale", "transposition l-a"},
        {235, "flal", "fall", "Structural & Scale", "transposition l-a"},
        {236, "tlal", "tall", "Structural & Scale", "transposition l-a"},
        {237, "hnad", "hand", "Structural & Scale", "transposition n-a"},
        {238, "bnad", "band", "Structural & Scale", "transposition n-a"},
        {239, "lnad", "land", "Structural & Scale", "transposition n-a"},
        {240, "snad", "sand", "Structural & Scale", "transposition n-a"},
        {241, "caterpilar", "caterpillar", "Structural & Scale", "single l deletion"},
        {242, "caterpilr", "caterpillar", "Structural & Scale", "multi deletion"},
        {243, "cat", "caterpillar", "Structural & Scale", "extreme length (3 vs 11)"},
        {244, "administrtion", "administration", "Structural & Scale", "omitted a"},
        {245, "admin", "administration", "Structural & Scale", "extreme length (5 vs 14)"},
        {246, "profesor", "professor", "Structural & Scale", "single s"},
        {247, "professir", "professor", "Structural & Scale", "i -> o slip"},
        {248, "doc", "doctor", "Structural & Scale", "truncation (3 vs 6)"},
        {249, "scholl", "school", "Structural & Scale", "transposition & extra l"},
        {250, "algoritm", "algorithm", "Structural & Scale", "omitted h"}
    };

    ofstream testCasesFile("test_cases.txt");
    testCasesFile << "================================================================================\n";
    testCasesFile << "              SPELLCHECK BENCHMARK TEST SUITE (250 SAMPLES)\n";
    testCasesFile << "================================================================================\n";
    testCasesFile << left << setw(6) << "ID" 
                  << setw(18) << "Query" 
                  << setw(18) << "Ground Truth" 
                  << setw(35) << "Category" 
                  << "Scenario\n";
    testCasesFile << string(95, '-') << "\n";

    for (const auto& tc : testCases) {
        testCasesFile << left << setw(6) << tc.id
                      << setw(18) << tc.query
                      << setw(18) << tc.groundTruth
                      << setw(35) << tc.category
                      << tc.notes << "\n";
    }
    testCasesFile.close();

    vector<QueryResult> results;
    results.reserve(testCases.size());

    cout << "Running 250 test cases through dynamic weighting pipeline...\n";

    for (const auto& tc : testCases) {
        auto tStart = chrono::high_resolution_clock::now();
        vector<Node> topk = recomendation(tc.query, list);
        auto tEnd = chrono::high_resolution_clock::now();

        double lat = chrono::duration<double, micro>(tEnd - tStart).count();

        int matchedRank = 9999;
        float bScore = 1.0f;
        float wLevUsed = 0.70f, wKbdUsed = 0.30f;

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

    ofstream statsFile(outputFileName);
    statsFile << "================================================================================\n";
    statsFile << "               DYNAMIC WEIGHT SPELLCHECK EVALUATION REPORT\n";
    statsFile << "================================================================================\n";
    statsFile << "Total Test Samples Evaluated : " << testCases.size() << "\n";
    statsFile << "Candidate Lexicon Size       : " << list.size() << "\n";
    statsFile << "Top-K Evaluated              : " << K << "\n";
    statsFile << "================================================================================\n\n";

    statsFile << "--------------------------------------------------------------------------------\n";
    statsFile << "PART 1: INDIVIDUAL QUERY TRACE LOG (250 SAMPLES)\n";
    statsFile << "--------------------------------------------------------------------------------\n";
    statsFile << left << setw(5) << "ID"
              << setw(16) << "Query"
              << setw(16) << "Ground Truth"
              << setw(8)  << "Rank"
              << setw(12) << "Score"
              << setw(8)  << "W_lev"
              << setw(8)  << "W_kbd"
              << setw(14) << "Latency (us)"
              << "Status\n";
    statsFile << string(90, '-') << "\n";

    for (const auto& r : results) {
        string rankStr = (r.rank <= K) ? ("#" + to_string(r.rank)) : "FAIL";
        string statusStr = (r.rank == 1) ? "TOP-1" : ((r.rank <= K) ? "TOP-K" : "MISS");

        statsFile << left << setw(5) << r.tc.id
                  << setw(16) << r.tc.query
                  << setw(16) << r.tc.groundTruth
                  << setw(8)  << rankStr
                  << fixed << setprecision(4) << setw(12) << r.bestScore
                  << setprecision(2) << setw(8) << r.wLev
                  << setw(8) << r.wKbd
                  << setprecision(1) << setw(14) << r.latencyUs
                  << statusStr << "\n";
    }

    statsFile << "\n--------------------------------------------------------------------------------\n";
    statsFile << "PART 2: CATEGORY-WISE STATISTICAL BREAKDOWN\n";
    statsFile << "--------------------------------------------------------------------------------\n";
    statsFile << left << setw(38) << "Category"
              << setw(9)  << "Samples"
              << setw(12) << "Top-1 Acc"
              << setw(12) << "Top-K Acc"
              << setw(10) << "MRR"
              << setw(10) << "Avg W_lev"
              << setw(10) << "Avg W_kbd"
              << "Avg Lat (us)\n";
    statsFile << string(110, '-') << "\n";

    vector<string> categories = {
        "Adjacent Horizontal Slips",
        "Adjacent Vertical/Diagonal Slips",
        "Single Deletions",
        "Single Insertions",
        "Structural & Scale"
    };

    int globalTop1 = 0;
    int globalTop5 = 0;
    double globalMrrSum = 0.0;
    double globalLatSum = 0.0;
    double globalWlevSum = 0.0;
    double globalWkbdSum = 0.0;

    for (const auto& cat : categories) {
        int count = 0;
        int top1 = 0;
        int top5 = 0;
        double mrr = 0.0;
        double sumWlev = 0.0;
        double sumWkbd = 0.0;
        double sumLat = 0.0;

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
        double catMrr = (count > 0) ? (mrr / count) : 0.0;

        // Correct declaration of average metrics per category
        double avgWlev = (count > 0) ? (sumWlev / count) : 0.0;
        double avgWkbd = (count > 0) ? (sumWkbd / count) : 0.0;
        double avgLatency = (count > 0) ? (sumLat / count) : 0.0;

        globalTop1 += top1;
        globalTop5 += top5;
        globalMrrSum += mrr;
        globalLatSum += sumLat;
        globalWlevSum += sumWlev;
        globalWkbdSum += sumWkbd;

        statsFile << left << setw(38) << cat
                  << setw(9)  << count
                  << fixed << setprecision(1) << setw(12) << (to_string((int)catTop1Acc) + "%")
                  << setw(12) << (to_string((int)catTop5Acc) + "%")
                  << setprecision(3) << setw(10) << catMrr
                  << setprecision(2) << setw(10) << avgWlev
                  << setw(10) << avgWkbd
                  << setprecision(1) << avgLatency << "\n";
    }

    double totalCount = static_cast<double>(results.size());
    double finalTop1Acc = (totalCount > 0) ? (globalTop1 / totalCount * 100.0) : 0.0;
    double finalTop5Acc = (totalCount > 0) ? (globalTop5 / totalCount * 100.0) : 0.0;
    double finalMrr = (totalCount > 0) ? (globalMrrSum / totalCount) : 0.0;
    double finalAvgLatency = (totalCount > 0) ? (globalLatSum / totalCount) : 0.0;
    double finalAvgWlev = (totalCount > 0) ? (globalWlevSum / totalCount) : 0.0;
    double finalAvgWkbd = (totalCount > 0) ? (globalWkbdSum / totalCount) : 0.0;

    statsFile << "\n--------------------------------------------------------------------------------\n";
    statsFile << "PART 3: GLOBAL PERFORMANCE SUMMARY\n";
    statsFile << "--------------------------------------------------------------------------------\n";
    statsFile << "Total Test Samples Run          : " << results.size() << "\n";
    statsFile << "Top-1 Accuracy                 : " << fixed << setprecision(2) << finalTop1Acc << "%\n";
    statsFile << "Top-3 Accuracy                 : " << fixed << setprecision(2) << finalTop5Acc << "%\n";
    statsFile << "Mean Reciprocal Rank (MRR)      : " << setprecision(4) << finalMrr << "\n";
    statsFile << "Average Pipeline Latency        : " << setprecision(2) << finalAvgLatency << " microseconds\n";
    statsFile << "Average Weight Balance          : W_lev = " << setprecision(2) << finalAvgWlev 
              << " | W_kbd = " << finalAvgWkbd << "\n";
    statsFile << "================================================================================\n";

    statsFile.close();

    cout << "\nBenchmark complete!\n";
    cout << "1. Completed with 250 categorized test queries.\n";
    cout << "2. Generated output file with detailed trace and statistical breakdown.\n";
    cout << "Overall Top-1: " << finalTop1Acc << "% | Top-3: " << finalTop5Acc << "% | MRR: " << finalMrr << "\n";

    return 0;
}