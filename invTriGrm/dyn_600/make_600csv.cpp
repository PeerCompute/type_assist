// generate_test_cases_600.cpp
// Generates the 600-case spellchecker benchmark (IDs 0-599) + its lexicon.
//
// Build : g++ -O2 -std=c++17 generate_test_cases_600.cpp -o gen600
// Run   : ./gen600 [output_dir] [seed]
// Output: <output_dir>/test_cases_600.csv   (id,query,ground_truth,category,type,truth_len,notes)
//         <output_dir>/lexicon_600.txt
//
// Layout (120 cases per category):
//   Adjacent Horizontal Slips : 112 normal + 8 EDGE
//   Vertical/Diagonal Slips   : 112 normal + 8 EDGE
//   Single Deletions          : 112 normal + 8 EDGE
//   Single Insertions         : 112 normal + 8 EDGE
//   Structural & Phonetic     : 36 transpositions + 40 phonetic + 18 truncations
//                               + 18 double errors + 8 EDGE (gibberish / limits)
// Normal cases are spread over 5 length bands (3-4, 5-6, 7-8, 9-11, 12+).
// EDGE cases have ground truth "<NONE>" (correct answer is not in the lexicon).

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <random>
#include <optional>
#include <cmath>
#include <cstdlib>

using namespace std;

// ============================ word lists ====================================
static const char* POOL_TXT =
R"W(the and cat dog sun run pen bed hat map cup bus )W"
R"W(key box car sit hot gold milk door fish tree rain song )W"
R"W(book walk talk lake farm road rice wind snow ship bird king )W"
R"W(moon star fire hand head foot nose help hold home time life )W"
R"W(look cake card care case cash cave chat chef cell cent mood )W"
R"W(food good wood fast last past cost post most host wall call )W"
R"W(fall tall ball band land sand dark park bark work word mark )W"
R"W(calm came camp cheek cold told held water green table house plant )W"
R"W(chair bread light money story music river stone sugar tiger window garden )W"
R"W(market simple sample friend family school street yellow orange purple player action )W"
R"W(nation person people system method vector string middle bottle little settle handle )W"
R"W(candle bundle butter better bitter letter winter summer spring island nature forest )W"
R"W(bridge police doctor lawyer artist officer coffee cheese circle church clock closet )W"
R"W(collar choice choose cheque cheer chess chest chief child chill china kitchen )W"
R"W(morning evening holiday journey command college climate circuit citizen classic comfort complete )W"
R"W(consider computer chicken country history science teacher student program network machine monitor )W"
R"W(library justice balance battery capital culture mountain testing running walking talking reading )W"
R"W(writing playing feeling looking working holding helping opening closing movement judgment payment )W"
R"W(shipment agreement treatment station section mention fraction reaction position condition relation solution )W"
R"W(decision provision revision soldier author swimming planning beginning building important different difficult )W"
R"W(necessary president government algorithm candidate keyboard distance correction similarity professor development environment )W"
R"W(information beautiful chocolate community dangerous education adventure restaurant experience temperature university technology )W"
R"W(literature vocabulary particular dictionary excellent apartment basketball celebration definition description engineering expression )W"
R"W(foundation generation imagination introduction mathematics operation population production situation statement structure suggestion )W"
R"W(tournament vegetable wonderful yesterday architecture laboratory caterpillar performance coefficient organization communication transportation )W"
R"W(understanding programming administration international responsibility characteristic environmental professional entertainment independence extraordinary approximately )W"
R"W(infrastructure comprehensive consideration congratulations conversation determination discrimination documentation establishment fundamental identification implementation )W"
R"W(investigation justification manufacturing neighborhood participation psychological recommendation representative relationship satisfaction significantly simultaneously )W"
R"W(sophisticated telecommunication transformation unfortunately accomplishment accountability archaeological collaboration constitutional demonstration )W";

static const char* EXTRAS_TXT =
R"W(cut can cap cab pin pat bat rat fat mat log fog )W"
R"W(bun fun big bag beg bug dig fig fix fox for far )W"
R"W(fur tea ten tin ton top tip tap two toe tie then )W"
R"W(than that this they them with what when were will would could )W"
R"W(should about after again other there their these those which where while )W"
R"W(pot pet net nut set sat bit hit hut hug hum jam )W"
R"W(jar jet joy lap leg lip lot mud nap net oil owl )W"
R"W(pad pan pie pig pit pop pub rag rub sad sea sip )W"
R"W(sky sob tub van wet wig win zip cane cone cube cure )W"
R"W(date dawn deal deep desk dish dive done dose down draw drop )W"
R"W(drum dust duty earn ease east edge else face fact fail fair )W"
R"W(farm feel feet fell file fill film find fine firm flag flat )W"
R"W(flow fold fork form free from fuel full gain game gate gift )W"
R"W(girl give glad goal goes grab gray grew grow gulf hair half )W"
R"W(hall hang hard harm hate have heat heel hill hire hope hour )W"
R"W(huge hunt idea iron item join jump june keen keep kick kind )W"
R"W(knee lack lady laid lamp lane late lazy lead lean left lend )W"
R"W(less lift line link list live load loan lock long lord lose )W"
R"W(loss lost loud love luck made mail main make male many mass )W"
R"W(mate meal mean meat meet mess mild mile mind mine miss mode )W"
R"W(more move much must name near neck need news next nice nine )W"
R"W(none noon norm note nuts obey once only open oral over pace )W"
R"W(pack page paid pain pair pale palm path pick pile pine pink )W"
R"W(pipe plan play plot plus poem pole pond pool poor port pull )W"
R"W(pure push race rail rare rate read real rely rent rest rich )W"
R"W(ride ring rise risk rock role roll roof room root rope rose )W"
R"W(rule rush safe said sail sake sale salt same save seat seed )W"
R"W(seek seem seen self sell send sent ship shop shot show shut )W"
R"W(sick side sign silk sing sink size skin slip slow soft soil )W"
R"W(sold sole some soon sort soul spot step stay stem stop such )W"
R"W(suit sure swim tail take tale tank tape task team tear tell )W"
R"W(tend term test text than thin thus tide tied till tiny tone )W"
R"W(tool tour town trap tree trip true tube tune turn twin type )W"
R"W(unit upon used user vast very view vote wage wait wake wave )W"
R"W(weak wear week well went west what whom wide wife wild wine )W"
R"W(wing wire wise wish wolf wore worn wrap yard year zero zone )W"
R"W(about above agree ahead alarm alive allow alone along among angle angry )W"
R"W(apple arrow aside asset avoid awake award aware basic beach begin below )W"
R"W(bench birth black blade blame blank blind block blood board boost bound )W"
R"W(brain brand brave break brick brief bring broad brown brush build burst )W"
R"W(buyer cable carry catch cause chain chalk charm chart chase cheap check )W"
R"W(chose civil claim class clean clear click climb close cloud coach coast )W"
R"W(count court cover crack craft crash crazy cream crime cross crowd crown )W"
R"W(curve daily dance death delay depth dirty doubt dozen draft drama dream )W"
R"W(dress drink drive eager early earth eight elect empty enemy enjoy enter )W"
R"W(entry equal error event every exact exist extra faith false fault fiber )W"
R"W(field fifth fifty fight final first fixed flame flash fleet floor focus )W"
R"W(force frame frank fresh front fruit fully funny ghost giant given glass )W"
R"W(globe grace grade grain grand grant grass great gross group grown guard )W"
R"W(guess guest guide happy heart heavy hence honey horse hotel human ideal )W"
R"W(image index inner input issue joint judge juice knife known label large )W"
R"W(laser later laugh layer learn lease least leave legal level limit local )W"
R"W(logic loose lower loyal lucky lunch magic major maker march match maybe )W"
R"W(mayor media metal might minor minus mixed model month moral motor mouth )W"
R"W(movie multi newly noise north novel nurse occur ocean offer often order )W"
R"W(other ought paint panel paper party peace phase phone photo piece pilot )W"
R"W(pitch place plain plane plate point pound power press price pride prime )W"
R"W(print prior prize proof proud prove queen quick quiet quite radio raise )W"
R"W(range rapid ratio reach ready realm rebel refer relax reply rider ridge )W"
R"W(right rigid rival river robot rocky roman rough round route royal rural )W"
R"W(scale scene scope score sense serve seven shade shake shall shape share )W"
R"W(sharp sheet shelf shell shift shine shirt shock shoot short shown sight )W"
R"W(sixth sixty sized skill sleep slide small smart smile smoke solid solve )W"
R"W(sorry sound south space spare speak speed spend spent split spoke sport )W"
R"W(staff stage stake stand start state steam steel steep stick still stock )W"
R"W(stood store storm strip stuck study stuff style sugar suite super sweet )W"
R"W(table taken taste taxes teach thank theme thick thing think third those )W"
R"W(three threw throw tight title today topic total touch tough tower track )W"
R"W(trade trail train treat trend trial tribe trick tried tries truck truly )W"
R"W(trust truth twice under union unity until upper upset urban usage usual )W"
R"W(valid value video virus visit vital voice waste watch wheel white whole )W"
R"W(whose woman women world worry worse worst worth would wound write wrong )W"
R"W(wrote young youth bridge cheese circle coffee couple course cousin credit crisis )W"
R"W(custom damage danger debate decade defend degree demand design detail device dinner )W"
R"W(direct divide double driver during easily effect effort either emerge enable energy )W"
R"W(engine enough ensure entire escape evolve exceed expand expect expert export extend )W"
R"W(factor fairly fallen famous father fellow figure finger finish flight flower follow )W"
R"W(forced forest forget formal format former foster fourth freedom friend future gather )W"
R"W(gender gentle german global golden ground growth guilty handle happen hardly health )W"
R"W(height hidden highly hockey honest hunger hungry impact import income indeed inform )W"
R"W(injury inside insist intend invest island itself jacket joined junior kidney killer )W"
R"W(launch lawyer leader league legacy length lesson letter liberal likely listen living )W"
R"W(losing lovely lonely lumber manage manner margin master matter mature medium member )W"
R"W(memory mental merely method middle minute mirror modern modest moment mother motion )W"
R"W(moving murder muscle mutual narrow nation nature nearby nearly needle nobody normal )W"
R"W(notice notion number object obtain occupy office online option orange origin output )W"
R"W(oxygen palace parent partly pass patent people period permit person phrase pickup )W"
R"W(planet player please plenty pocket poetry police policy pollen portal poverty powder )W"
R"W(prefer pretty prince prison profit proper public purple pursue rabbit rather rating )W"
R"W(reader really reason recall recent record reduce reform refuse regard region reject )W"
R"W(relate relief remain remote remove repair repeat report rescue resist resort result )W"
R"W(retail retain retire return reveal review reward riding rising robust rocket rubber )W"
R"W(ruling sacred safety salary sample school screen search season second secret sector )W"
R"W(secure seeing select senior series server settle severe shadow shield should shower )W"
R"W(signal silent silver simple simply single sister slight smooth social solely sudden )W"
R"W(suffer summer summit supply surely survey switch symbol system tablet target temple )W"
R"W(tennis thanks theory thirty though thread threat throat ticket timber tissue toward )W"
R"W(travel treaty trying tunnel twelve twenty unable unique unless update useful valley )W"
R"W(vendor versus victim vision visual volume walker wallet wander wealth weapon weekly )W"
R"W(weight widely window winner winter wisdom within wonder wooden worker worthy yellow )W"
R"W(kingdom kitchen lecture leather library lightly limited machine manager married massive meeting )W"
R"W(message million mineral minimum minister mission mixture monitor monthly morning mystery natural )W"
R"W(neither network nothing nuclear nursing officer opening operate opinion organic outcome outside )W"
R"W(overall painful painter parking partner passage passion patient pattern payment penalty pension )W"
R"W(perfect perform perhaps persist phrases physics picture plastic pleased pointed popular portion )W"
R"W(possess poverty predict premium prepare present prevent primary printer privacy private problem )W"
R"W(proceed process produce product profile program project promise promote protect protein protest )W"
R"W(provide purpose pursuit quality quarter quickly rainbow rapidly rarely reading reality receive )W"
R"W(recover reflect refugee regular related release remains remarks removal replace request require )W"
R"W(reserve resolve respect respond restore revenue reverse routine running science scratch section )W"
R"W(segment select seventh several shallow shelter shortly shoulder silence similar simplify sitting )W"
R"W(skilled slowly society soldier somehow someone speaker special species spirits sponsor squares )W"
R"W(standard station storage strange stretch student studies subject succeed success suggest summary )W"
R"W(support suppose supreme surface surplus suspect sustain teacher tension theater therapy thereby )W"
R"W(thought through tonight tourist towards trading traffic trained trainer transit trouble trustee )W"
R"W(turning ukraine unified uniform unknown unusual upgrade useless variety various vehicle venture )W"
R"W(version veteran victory village violent virtual visible visitor voltage walking warning weekend )W"
R"W(welcome western whereas whether willing windows winning wishing without witness working workout )W"
R"W(worried writing written yielded planned planner planning builder building swimmer swimming beginner )W"
R"W(beginning authors authority soldiers computers computing complete completely complex component compose compound )W"
R"W(compress comprise compute concept concern concert conclude concrete condemn conduct confirm conflict )W"
R"W(confuse connect consent consist console consume contact contain content contest context contract )W"
R"W(control convert convince cooking cooling copper correct correctly corridor counter country covered )W"
R"W(cowboy crystal cultural curious current cursor cutting accept access accident account accurate )W"
R"W(achieve acquire address adjust admire admit adopt adult advance advice advise affair )W"
R"W(affect afford afraid against agency agenda almost already although always amount ancient )W"
R"W(animal annual another answer anxiety anybody anymore anyone anything anyway appeal appear )W"
R"W(applied apply approach approve around arrange arrive article artist asking aspect assault )W"
R"W(assess assign assist assume attach attack attempt attend attract august author autumn )W"
R"W(average avoid awkward backup balance ballot banner barely barrel basket battle beauty )W"
R"W(because become before behalf behave behind belief belong beside better between beyond )W"
R"W(bicycle billion binary biology bishop bitter blanket blossom border borrow bother bottle )W"
R"W(bottom bounce boxing branch breath breeze bright broken brother budget buffer bullet )W"
R"W(bundle burden bureau button camera campus cancel cancer candle cannot canvas capable )W"
R"W(capital captain capture carbon career careful carpet carrier casino castle casual cattle )W"
R"W(caught ceiling center central century certain chamber chance change channel chapter charge )W"
R"W(charity cheerful chicken chief childhood chimney choice choose chosen church cinema circuit )W"
R"W(citizen classic classify climate closer closest clothes cluster coastal collect college colonel )W"
R"W(column combine comfort command comment commit common compact company compare compete )W";

static const vector<pair<string,string>> PHON = {
    {"fone","phone"}, {"nife","knife"}, {"rite","write"}, {"nite","night"},
    {"lite","light"}, {"tuff","tough"}, {"wich","which"}, {"wimen","women"},
    {"tounge","tongue"}, {"enuff","enough"}, {"kwality","quality"}, {"sience","science"},
    {"sycology","psychology"}, {"grammer","grammar"}, {"calender","calendar"}, {"libary","library"},
    {"tomorow","tomorrow"}, {"wensday","wednesday"}, {"febuary","february"}, {"seperate","separate"},
    {"definately","definitely"}, {"recieve","receive"}, {"occured","occurred"}, {"necesary","necessary"},
    {"embarass","embarrass"}, {"accomodate","accommodate"}, {"cemetary","cemetery"}, {"intresting","interesting"},
    {"favrite","favorite"}, {"diffrent","different"}, {"enviroment","environment"}, {"probly","probably"},
    {"rythm","rhythm"}, {"sissors","scissors"}, {"neumonia","pneumonia"}, {"lisence","license"},
    {"exagerate","exaggerate"}, {"comittee","committee"}, {"pronounciation","pronunciation"}, {"buisness","business"},
};

static const vector<pair<string,string>> TRUNC = {
    {"caterp","caterpillar"}, {"admin","administration"}, {"prof","professor"}, {"doct","doctor"},
    {"commun","communication"}, {"enviro","environment"}, {"organiz","organization"}, {"transp","transportation"},
    {"algo","algorithm"}, {"devel","development"}, {"informa","information"}, {"progra","programming"},
    {"underst","understanding"}, {"respons","responsibility"}, {"charact","characteristic"}, {"indepen","independence"},
    {"chocol","chocolate"}, {"educ","education"},
};

// out-of-lexicon words used for "no suitable result" edge cases (8 per category)
static const vector<string> OOV_H = {"kiwi","zebra","quokka","pharaoh","xylophone","kubernetes","bougainvillea","onomatopoeia"};
static const vector<string> OOV_V = {"yoga","llama","zeppelin","pyramid","hieroglyph","quarantine","archipelago","hippopotamus"};
static const vector<string> OOV_D = {"yeti","koala","walrus","tsunami","blockchain","espresso","chrysanthemum","rhododendron"};
static const vector<string> OOV_I = {"taco","panda","sphinx","iguana","jellyfish","avalanche","extraterrestrial","thermodynamics"};
static const vector<string> GIBBERISH = {"qzxjv","wxqz","mmmmm","plkjhgf","asdfghjkl","zzyx","q",
                                         "thequickbrownfoxjumpsoverthelazydogagain"};

static const string C1 = "Adjacent Horizontal Slips";
static const string C2 = "Vertical/Diagonal Slips";
static const string C3 = "Single Deletions";
static const string C4 = "Single Insertions";
static const string C5 = "Structural & Phonetic";

// ============================ helpers =======================================
static mt19937 rng;
static double rnd01() { return uniform_real_distribution<double>(0.0, 1.0)(rng); }
static int rndInt(int lo, int hi) { return uniform_int_distribution<int>(lo, hi)(rng); } // inclusive
template <class T> static void shuffleVec(vector<T>& v) { shuffle(v.begin(), v.end(), rng); }

static vector<string> splitWords(const string& s) {
    vector<string> out; istringstream is(s); string w;
    while (is >> w) out.push_back(w);
    return out;
}

// --- keyboard geometry (staggered QWERTY; row offsets 0 / 0.25 / 0.75 key widths)
static const string ROWS[3] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
static const double OFFS[3] = {0.0, 0.25, 0.75};
static bool keyPos(char c, double& x, double& y) {
    for (int r = 0; r < 3; ++r) {
        size_t i = ROWS[r].find(c);
        if (i != string::npos) { x = OFFS[r] + (double)i; y = r; return true; }
    }
    return false;
}
static vector<char> hneigh(char c) {            // same row, left/right
    vector<char> out;
    for (int r = 0; r < 3; ++r) {
        size_t i = ROWS[r].find(c);
        if (i == string::npos) continue;
        if (i > 0) out.push_back(ROWS[r][i - 1]);
        if (i + 1 < ROWS[r].size()) out.push_back(ROWS[r][i + 1]);
    }
    return out;
}
static vector<char> vneigh(char c) {            // adjacent row, within 1.3 key widths
    vector<char> out; double x, y;
    if (!keyPos(c, x, y)) return out;
    for (int r = 0; r < 3; ++r)
        for (size_t i = 0; i < ROWS[r].size(); ++i) {
            double a = OFFS[r] + (double)i, b = r;
            if (fabs(b - y) == 1.0 && sqrt((a - x) * (a - x) + (b - y) * (b - y)) <= 1.3)
                out.push_back(ROWS[r][i]);
        }
    return out;
}
static vector<char> aneigh(char c) {
    vector<char> a = hneigh(c), b = vneigh(c);
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

static int lev(const string& a, const string& b) {
    vector<int> dp(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) dp[j] = (int)j;
    for (size_t i = 1; i <= a.size(); ++i) {
        int prev = dp[0]; dp[0] = (int)i;
        for (size_t j = 1; j <= b.size(); ++j) {
            int t = dp[j];
            dp[j] = min({dp[j] + 1, dp[j - 1] + 1, prev + (a[i - 1] != b[j - 1] ? 1 : 0)});
            prev = t;
        }
    }
    return dp[b.size()];
}

static int bucketOf(size_t len) { return len <= 4 ? 0 : len <= 6 ? 1 : len <= 8 ? 2 : len <= 11 ? 3 : 4; }

// ============================ global state ==================================
static set<string> LEX;
static set<string> USED_Q;
static int CNT[26];                       // per-category cap on slip / inserted letters

struct Row { string query, truth, category, note; };
struct Made { string q, note; char n; };  // n = typed/inserted letter (0 if none)

static bool okQ(const string& q, const string& w) {
    if (q == w || q.size() < 2) return false;
    if (LEX.count(q) || USED_Q.count(q)) return false;
    for (char c : q) if (c < 'a' || c > 'z') return false;
    return true;
}

// ============================ typo makers ===================================
static optional<Made> mkH(const string& w) {
    vector<pair<int,char>> c;
    for (int i = 0; i < (int)w.size(); ++i)
        for (char n : hneigh(w[i])) if (i > 0 || rnd01() < 0.1) c.push_back({i, n});
    shuffleVec(c);
    for (auto& [i, n] : c) {
        string q = w; q[i] = n;
        if (okQ(q, w) && CNT[n - 'a'] < 9)
            return Made{q, string("typed ") + n + " for " + w[i] + " (same row)", n};
    }
    return nullopt;
}
static optional<Made> mkV(const string& w) {
    vector<pair<int,char>> c;
    for (int i = 0; i < (int)w.size(); ++i)
        for (char n : vneigh(w[i])) if (i > 0 || rnd01() < 0.1) c.push_back({i, n});
    shuffleVec(c);
    for (auto& [i, n] : c) {
        string q = w; q[i] = n;
        if (okQ(q, w) && CNT[n - 'a'] < 9)
            return Made{q, string("typed ") + n + " for " + w[i] + " (row above/below)", n};
    }
    return nullopt;
}
static optional<Made> mkD(const string& w) {
    vector<int> c;
    for (int i = 0; i < (int)w.size(); ++i) if (i > 0 || rnd01() < 0.1) c.push_back(i);
    shuffleVec(c);
    for (int i : c) {
        string q = w.substr(0, i) + w.substr(i + 1);
        if (okQ(q, w))
            return Made{q, string("omitted ") + w[i] + " at pos " + to_string(i + 1), 0};
    }
    return nullopt;
}
static optional<Made> mkI(const string& w) {
    vector<pair<int,char>> c;
    int L = (int)w.size();
    for (int p = 1; p <= L; ++p) {
        if (p == L && rnd01() > 0.15) continue;           // rarely append at the very end
        set<char> ns;
        for (char x : aneigh(w[p - 1])) ns.insert(x);
        if (p < L) for (char x : aneigh(w[p])) ns.insert(x);
        for (char n : ns)
            if (n != w[p - 1] && (p == L || n != w[p])) c.push_back({p, n});
    }
    shuffleVec(c);
    for (auto& [p, n] : c) {
        string q = w.substr(0, p) + n + w.substr(p);
        if (okQ(q, w) && CNT[n - 'a'] < 9) {
            vector<char> an = aneigh(w[p - 1]);
            char nb = (find(an.begin(), an.end(), n) != an.end()) ? w[p - 1] : w[p];
            string nxt = (p < L) ? string(1, w[p]) : string("_");
            return Made{q, string("extra ") + n + " between " + w[p - 1] + " and " + nxt +
                           " (" + n + " is next to " + nb + ")", n};
        }
    }
    return nullopt;
}
static optional<Made> mkT(const string& w) {
    vector<int> c;
    for (int i = 0; i + 1 < (int)w.size(); ++i)
        if (w[i] != w[i + 1] && (i > 0 || rnd01() < 0.1)) c.push_back(i);
    shuffleVec(c);
    for (int i : c) {
        string q = w; swap(q[i], q[i + 1]);
        if (okQ(q, w)) return Made{q, string("swapped ") + w[i] + w[i + 1], 0};
    }
    return nullopt;
}
static optional<Made> mkDbl(const string& w) {          // one neighbouring-key slip + one omission
    int L = (int)w.size();
    for (int tries = 0; tries < 80; ++tries) {
        int i = rndInt(1, L - 1), j = rndInt(1, L - 1);
        if (abs(i - j) < 2) continue;
        vector<char> ns = aneigh(w[i]);
        if (ns.empty()) continue;
        char n = ns[rndInt(0, (int)ns.size() - 1)];
        string t = w; t[i] = n;
        string q = t.substr(0, j) + t.substr(j + 1);
        if (okQ(q, w))
            return Made{q, string("typed ") + n + " for " + w[i] + " + omitted " + w[j], 0};
    }
    return nullopt;
}

typedef optional<Made> (*Maker)(const string&);

// ============================ builders ======================================
static vector<Row> build(const string& cat, Maker mk, const int quota[5],
                         const vector<string> pools[5], bool useCap) {
    fill(begin(CNT), end(CNT), 0);
    vector<Row> out;
    for (int b = 0; b < 5; ++b) {
        vector<string> cand = pools[b];
        shuffleVec(cand);
        int got = 0;
        for (auto& w : cand) {
            if (got >= quota[b]) break;
            auto r = mk(w);
            if (!r) continue;
            if (useCap && r->n) CNT[r->n - 'a']++;
            USED_Q.insert(r->q);
            out.push_back({r->q, w, cat, r->note});
            ++got;
        }
        if (got != quota[b]) {
            cerr << "ERROR: could not fill quota for " << cat << " band " << b
                 << " (" << got << "/" << quota[b] << ")\n";
            exit(1);
        }
    }
    return out;
}

static vector<Row> edgeRows(const string& cat, Maker mk, const vector<string>& words) {
    fill(begin(CNT), end(CNT), 0);
    vector<Row> out;
    for (auto& w : words) {
        if (LEX.count(w)) { cerr << "ERROR: OOV word is in lexicon: " << w << "\n"; exit(1); }
        optional<Made> r;
        for (int t = 0; t < 50 && !r; ++t) r = mk(w);
        if (!r) { cerr << "ERROR: no typo for edge word " << w << "\n"; exit(1); }
        USED_Q.insert(r->q);
        out.push_back({r->q, "<NONE>", cat, "EDGE intended '" + w + "' (not in lexicon); " + r->note});
    }
    return out;
}

static string classify(const Row& r) {
    if (r.truth == "<NONE>") return "EDGE";
    if (r.note.rfind("PHONETIC", 0) == 0) return "PHONETIC";
    int d = lev(r.query, r.truth);
    bool tie = false;
    for (auto& w : LEX) if (w != r.truth && lev(r.query, w) <= d) { tie = true; break; }
    if (tie) return "HARD";
    return r.truth.size() >= 9 ? "BEST" : "NORMAL";
}

static string csvField(const string& s) {
    if (s.find_first_of(",\"\n") == string::npos) return s;
    string o = "\"";
    for (char c : s) { if (c == '"') o += '"'; o += c; }
    return o + "\"";
}

// ============================ main ==========================================
int main(int argc, char** argv) {
    string outDir = argc > 1 ? argv[1] : ".";
    unsigned seed = argc > 2 ? (unsigned)strtoul(argv[2], nullptr, 10) : 20261103u;
    rng.seed(seed);

    // ---- lexicon -----------------------------------------------------------
    vector<string> poolV = splitWords(POOL_TXT);
    set<string> pool(poolV.begin(), poolV.end());
    for (auto& w : splitWords(EXTRAS_TXT)) LEX.insert(w);
    for (auto& w : pool) LEX.insert(w);
    for (auto& p : PHON)  LEX.insert(p.second);
    for (auto& p : TRUNC) LEX.insert(p.second);
    for (const char* w : {"ukraine", "german", "roman", "june", "august", "january"}) LEX.erase(w);
    set<string> oovAll;
    for (auto* v : {&OOV_H, &OOV_V, &OOV_D, &OOV_I}) for (auto& w : *v) oovAll.insert(w);
    for (auto& w : oovAll) LEX.erase(w);
    for (auto it = LEX.begin(); it != LEX.end();) {
        bool bad = it->empty();
        for (char c : *it) if (c < 'a' || c > 'z') bad = true;
        it = bad ? LEX.erase(it) : next(it);
    }
    for (auto& g : GIBBERISH) if (LEX.count(g)) { cerr << "gibberish in lexicon: " << g << "\n"; return 1; }

    vector<string> B[5];
    for (auto& w : pool) if (LEX.count(w)) B[bucketOf(w.size())].push_back(w);
    cout << "bucket sizes:";
    for (int b = 0; b < 5; ++b) cout << " " << B[b].size();
    cout << "\n";

    const int QUOTA[5] = {22, 22, 22, 23, 23};
    vector<Row> c1 = build(C1, mkH, QUOTA, B, true);
    { auto e = edgeRows(C1, mkH, OOV_H); c1.insert(c1.end(), e.begin(), e.end()); }
    vector<Row> c2 = build(C2, mkV, QUOTA, B, true);
    { auto e = edgeRows(C2, mkV, OOV_V); c2.insert(c2.end(), e.begin(), e.end()); }
    vector<Row> c3 = build(C3, mkD, QUOTA, B, false);
    { auto e = edgeRows(C3, mkD, OOV_D); c3.insert(c3.end(), e.begin(), e.end()); }
    vector<Row> c4 = build(C4, mkI, QUOTA, B, true);
    { auto e = edgeRows(C4, mkI, OOV_I); c4.insert(c4.end(), e.begin(), e.end()); }

    // ---- category 5 --------------------------------------------------------
    vector<Row> c5;
    const int TQ[5] = {8, 7, 7, 7, 7};                       // 36 transpositions
    { auto t = build(C5, mkT, TQ, B, false); c5.insert(c5.end(), t.begin(), t.end()); }
    for (auto& [q, t] : PHON) {                               // 40 phonetic
        if (LEX.count(q) || !LEX.count(t)) { cerr << "bad phonetic pair " << q << "/" << t << "\n"; return 1; }
        USED_Q.insert(q); c5.push_back({q, t, C5, "PHONETIC sounds right but spelled wrong"});
    }
    for (auto& [q, t] : TRUNC) {                              // 18 truncations / length imbalance
        if (LEX.count(q) || !LEX.count(t)) { cerr << "bad truncation pair " << q << "/" << t << "\n"; return 1; }
        USED_Q.insert(q);
        c5.push_back({q, t, C5, "LENGTH truncated " + to_string(q.size()) + " vs " + to_string(t.size())});
    }
    {                                                         // 18 double errors (slip + omission)
        const int DQ[5] = {0, 0, 6, 6, 6};
        vector<string> DP[5];
        for (int b = 0; b < 5; ++b) for (auto& w : B[b]) if (w.size() >= 6) DP[b].push_back(w);
        auto d = build(C5, mkDbl, DQ, DP, false);
        for (auto& r : d) { r.note = "DOUBLE " + r.note; c5.push_back(r); }
    }
    for (auto& g : GIBBERISH) {                               // 8 edge cases
        USED_Q.insert(g);
        c5.push_back({g, "<NONE>", C5, "EDGE gibberish / keyboard mash / length limit"});
    }

    // ---- assemble, shuffle inside each category, validate -------------------
    vector<Row> all;
    for (auto* part : {&c1, &c2, &c3, &c4, &c5}) {
        if (part->size() != 120) { cerr << "ERROR: category size " << part->size() << "\n"; return 1; }
        shuffleVec(*part);
        all.insert(all.end(), part->begin(), part->end());
    }
    set<string> uq;
    for (auto& r : all) uq.insert(r.query);
    if (all.size() != 600 || uq.size() != 600) { cerr << "ERROR: total/unique mismatch\n"; return 1; }
    for (auto& r : all)
        if (r.truth != "<NONE>" && !LEX.count(r.truth)) { cerr << "ERROR: truth missing: " << r.truth << "\n"; return 1; }

    // ---- write files ----------------------------------------------------------
    map<string,int> typeCount;
    {
        ofstream f(outDir + "/test_cases_600.csv");
        if (!f) { cerr << "cannot write to " << outDir << "\n"; return 1; }
        f << "id,query,ground_truth,category,type,truth_len,notes\n";
        for (size_t i = 0; i < all.size(); ++i) {
            const Row& r = all[i];
            string ty = classify(r); typeCount[ty]++;
            f << i << "," << r.query << "," << r.truth << "," << csvField(r.category) << "," << ty << ","
              << (r.truth == "<NONE>" ? 0 : (int)r.truth.size()) << "," << csvField(r.note) << "\n";
        }
    }
    {
        ofstream f(outDir + "/lexicon_600.txt");
        for (auto& w : LEX) f << w << "\n";
    }
    cout << "lexicon size: " << LEX.size() << "\n";
    cout << "wrote 600 cases (ids 0-599) to " << outDir << "/test_cases_600.csv\n";
    for (auto& [k, v] : typeCount) cout << "  " << k << ": " << v << "\n";
    return 0;
}
