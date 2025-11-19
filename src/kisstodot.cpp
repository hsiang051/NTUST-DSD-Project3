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

    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input.kiss> <output.dot>\n";
        return 1;
    }

    string kissPath = argv[1];
    string dotPath  = argv[2];

    ifstream fin(kissPath);
    if (!fin) {
        cerr << "Error: cannot open input KISS file: " << kissPath << "\n";
        return 1;
    }

    int numInputs  = -1;
    int numOutputs = -1;
    int numProducts = -1;
    int numStatesDeclared = -1;
    string resetStateName;

    bool inKiss = false;
    vector<RawTransition> rawTransitions;
    set<string> stateNames;
    set<string> inputPatterns;

    string line;
    while (getline(fin, line)) {
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
                    stateNames.insert(resetStateName);
                }
            }
        } else if (inKiss) {
            string in, ps, ns, out;
            stringstream ss(line);
            if (!(ss >> in >> ps >> ns >> out)) {
                continue; // malformed line
            }
            rawTransitions.push_back({in, ps, ns, out});
            inputPatterns.insert(in);
            stateNames.insert(ps);
            stateNames.insert(ns);
        }
    }
    fin.close();

    if (resetStateName.empty()) {
        cerr << "Warning: no .r reset state found, DOT will have INIT -> <first state>.\n";
    }

    // Merge labels for same (present, next) pair: "input/output" with comma
    map<pair<string,string>, vector<string>> edgeLabels;
    for (const auto &rt : rawTransitions) {
        string label = rt.input + "/" + rt.output;
        edgeLabels[{rt.present, rt.next}].push_back(label);
    }

    ofstream fout(dotPath);
    if (!fout) {
        cerr << "Error: cannot open output DOT file: " << dotPath << "\n";
        return 1;
    }

    fout << "digraph STG {\n";
    fout << "   rankdir=LR;\n\n";
    fout << "   INIT [shape=point];\n";

    // Declare states
    for (const auto &name : stateNames) {
        fout << "   " << name << " [label=\"" << name << "\"];\n";
    }
    fout << "\n";

    // INIT edge
    string initTarget;
    if (!resetStateName.empty() && stateNames.count(resetStateName)) {
        initTarget = resetStateName;
    } else if (!stateNames.empty()) {
        initTarget = *stateNames.begin();
    }

    if (!initTarget.empty()) {
        fout << "   INIT -> " << initTarget << ";\n";
    }

    // Edges with labels
    for (const auto &kv : edgeLabels) {
        const string &from = kv.first.first;
        const string &to   = kv.first.second;
        const vector<string> &labels = kv.second;

        string labelJoined;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (i > 0) labelJoined += ",";
            labelJoined += labels[i];
        }
        fout << "   " << from << " -> " << to
             << " [label=\"" << labelJoined << "\"];\n";
    }

    fout << "}\n";
    fout.close();

    return 0;
}
