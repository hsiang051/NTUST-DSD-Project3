#include <bits/stdc++.h>
using namespace std;

struct RawTransition {
    string input;
    string present;
    string next;
    string output;
};

static inline string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <input.kiss> <output.kiss> <output.dot>\n";
        return 1;
    }

    string inputKissPath  = argv[1];
    string outputKissPath = argv[2];
    string outputDotPath  = argv[3];

    ifstream fin(inputKissPath);
    if (!fin) {
        cerr << "Error: cannot open input file: " << inputKissPath << "\n";
        return 1;
    }

    // KISS meta info
    int numInputs  = -1;
    int numOutputs = -1;
    int numProducts = -1; // .p
    int numStatesDeclared = -1; // .s
    string resetStateName;

    bool inKiss = false;
    vector<RawTransition> rawTransitions;
    map<string,int> stateIndex;
    vector<string> states;
    auto getStateId = [&](const string &name) -> int {
        auto it = stateIndex.find(name);
        if (it != stateIndex.end()) return it->second;
        int id = (int)states.size();
        states.push_back(name);
        stateIndex[name] = id;
        return id;
    };

    set<string> inputPatterns;

    string line;
    while (std::getline(fin, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line[0] == '.') {
            string token;
            stringstream ss(line);
            ss >> token;
            if (token == ".start_kiss") {
                inKiss = true;
                continue;
            } else if (token == ".end_kiss") {
                break;
            } else if (token == ".i") {
                ss >> numInputs;
            } else if (token == ".o") {
                ss >> numOutputs;
            } else if (token == ".p") {
                ss >> numProducts;
            } else if (token == ".s") {
                ss >> numStatesDeclared;
            } else if (token == ".r") {
                ss >> resetStateName;
                if (!resetStateName.empty()) {
                    getStateId(resetStateName);
                }
            }
        } else if (inKiss) {
            string in, ps, ns, out;
            stringstream ss(line);
            if (!(ss >> in >> ps >> ns >> out)) {
                continue;
            }
            RawTransition rt{in, ps, ns, out};
            rawTransitions.push_back(rt);

            inputPatterns.insert(in);
            getStateId(ps);
            getStateId(ns);
        }
    }
    fin.close();

    if (numInputs <= 0 || numOutputs <= 0 || resetStateName.empty()) {
        cerr << "Error: invalid or missing .i/.o/.r directives in KISS file.\n";
        return 1;
    }

    if (stateIndex.find(resetStateName) == stateIndex.end()) {
        getStateId(resetStateName);
    }

    const int nStates = (int)states.size();
    int resetId = stateIndex[resetStateName];

    // Build transition table: state -> (input -> (nextState, output))
    vector<unordered_map<string, pair<int,string>>> transTable(nStates);

    for (const auto &rt : rawTransitions) {
        int psId = stateIndex[rt.present];
        int nsId = stateIndex[rt.next];
        // Assume deterministic & completely specified, overwrite if duplicated
        transTable[psId][rt.input] = {nsId, rt.output};
    }

    // Create sorted list of input patterns
    vector<string> inputList(inputPatterns.begin(), inputPatterns.end());
    sort(inputList.begin(), inputList.end());

    // Compute reachable states from reset
    vector<bool> reachable(nStates, false);
    queue<int> q;
    reachable[resetId] = true;
    q.push(resetId);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const auto &kv : transTable[u]) {
            int v = kv.second.first;
            if (!reachable[v]) {
                reachable[v] = true;
                q.push(v);
            }
        }
    }

    // State minimization via partition refinement
    // part[i] = block id of state i
    vector<int> part(nStates, 0);

    bool changed = true;
    while (changed) {
        changed = false;
        vector<int> oldPart = part;

        unordered_map<string, int> sig2block;
        int nextBlockId = 0;

        for (int s = 0; s < nStates; ++s) {
            // Build signature for state s based on output & next block
            // Even unreachable states are included; they will be filtered later.
            string sig;
            for (const auto &inp : inputList) {
                auto it = transTable[s].find(inp);
                if (it == transTable[s].end()) {
                    // Should not happen for completely specified STG
                    sig += inp + ":#:#;";
                } else {
                    int ns = it->second.first;
                    const string &out = it->second.second;
                    sig += inp + ":" + out + ":" + to_string(oldPart[ns]) + ";";
                }
            }
            auto itb = sig2block.find(sig);
            int bid;
            if (itb == sig2block.end()) {
                bid = nextBlockId++;
                sig2block[sig] = bid;
            } else {
                bid = itb->second;
            }
            part[s] = bid;
        }

        if (part != oldPart) {
            changed = true;
        }
    }

    int maxBlock = 0;
    for (int v : part) maxBlock = max(maxBlock, v);
    int totalBlocks = maxBlock + 1;

    // Map blocks (equivalence classes) to new states (only reachable)
    vector<bool> blockReachable(totalBlocks, false);
    for (int s = 0; s < nStates; ++s) {
        if (reachable[s]) {
            blockReachable[part[s]] = true;
        }
    }

    // For each block, choose a representative reachable state (if any)
    vector<int> blockRepState(totalBlocks, -1);
    for (int s = 0; s < nStates; ++s) {
        int b = part[s];
        if (!blockReachable[b]) continue;
        if (blockRepState[b] == -1 && reachable[s]) {
            blockRepState[b] = s;
        }
    }

    // Assign new indices and names to reachable blocks
    vector<int> blockToNewIndex(totalBlocks, -1);
    vector<string> newStateNames;
    vector<int> newStateRep;

    for (int b = 0; b < totalBlocks; ++b) {
        if (!blockReachable[b]) continue;
        int rep = blockRepState[b];
        if (rep == -1) continue;
        int newIdx = (int)newStateNames.size();
        blockToNewIndex[b] = newIdx;
        newStateNames.push_back(states[rep]);
        newStateRep.push_back(rep);
    }

    const int nNewStates = (int)newStateNames.size();

    // Find new reset state name
    int resetBlock = part[resetId];
    int resetNewIndex = blockToNewIndex[resetBlock];
    string newResetName = newStateNames[resetNewIndex];

    // Build minimized transitions (KISS)
    struct MinTransition {
        string input;
        string present;
        string next;
        string output;
    };
    vector<MinTransition> minTrans;
    minTrans.reserve(nNewStates * inputList.size());

    for (int nb = 0; nb < nNewStates; ++nb) {
        int rep = newStateRep[nb];
        const string &presentName = newStateNames[nb];
        for (const auto &inp : inputList) {
            auto it = transTable[rep].find(inp);
            if (it == transTable[rep].end()) {
                continue;
            }
            int ns = it->second.first;
            const string &out = it->second.second;
            int destBlock = part[ns];
            int destNewIdx = blockToNewIndex[destBlock];
            if (destNewIdx < 0) {
                continue;
            }
            const string &destName = newStateNames[destNewIdx];
            minTrans.push_back({inp, presentName, destName, out});
        }
    }

    int newNumProducts = (int)minTrans.size();

    // Write output KISS
    ofstream foutK(outputKissPath);
    if (!foutK) {
        cerr << "Error: cannot open output KISS file: " << outputKissPath << "\n";
        return 1;
    }

    foutK << ".start_kiss\n";
    foutK << ".i " << numInputs << "\n";
    foutK << ".o " << numOutputs << "\n";
    foutK << ".p " << newNumProducts << "\n";
    foutK << ".s " << nNewStates << "\n";
    foutK << ".r " << newResetName << "\n";

    for (const auto &mt : minTrans) {
        foutK << mt.input << " " << mt.present << " " << mt.next << " " << mt.output << "\n";
    }

    foutK << ".end_kiss\n";
    foutK.close();

    // Build DOT graph
    // Merge labels for same edge
    map<pair<string,string>, vector<string>> edgeLabels;
    for (const auto &mt : minTrans) {
        string label = mt.input + "/" + mt.output;
        edgeLabels[{mt.present, mt.next}].push_back(label);
    }

    ofstream foutD(outputDotPath);
    if (!foutD) {
        cerr << "Error: cannot open output DOT file: " << outputDotPath << "\n";
        return 1;
    }

    foutD << "digraph STG {\n";
    foutD << "   rankdir=LR;\n\n";
    foutD << "   INIT [shape=point];\n";

    // State declarations
    for (const auto &name : newStateNames) {
        foutD << "   " << name << " [label=\"" << name << "\"];\n";
    }
    foutD << "\n";
    foutD << "   INIT -> " << newResetName << ";\n";

    // Edges
    for (const auto &kv : edgeLabels) {
        const string &from = kv.first.first;
        const string &to   = kv.first.second;
        const vector<string> &labels = kv.second;

        string labelJoined;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (i > 0) labelJoined += ",";
            labelJoined += labels[i];
        }
        foutD << "   " << from << " -> " << to << " [label=\"" << labelJoined << "\"];\n";
    }

    foutD << "}\n";
    foutD.close();

    return 0;
}
