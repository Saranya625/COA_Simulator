#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <set>
#include <stdexcept>

#define MEMORY_SIZE 4096  // 4KB shared memory
#define NUM_CORES 4       // 4 processor cores
#define REGISTERS 32      // 32 registers
#define CORE_MEMORY (MEMORY_SIZE / NUM_CORES)

using namespace std;

// Shared memory
uint8_t memory[MEMORY_SIZE] = {0};

// Convert "x1" -> 1, "x2" -> 2, ..., "x31" -> 31
int getRegisterIndex(const string &reg) {
    if (reg[0] == 'x') {
        int num = stoi(reg.substr(1));  
        if (num >= 0 && num < REGISTERS) return num;
    }
    throw invalid_argument("Invalid register: " + reg);
}

// Core structure
struct Core {
    int registers[REGISTERS] = {0};  
    const int core_id;               
    int pc = 0;                      
    set<int> visited_pcs;  // Track visited instructions

    Core(int id) : core_id(id) {
        registers[0] = 0; 
    }

    int lw(int address) {
        return *(int*)(memory + address);
    }

    void sw(int address, int value) {
        *(int*)(memory + address) = value;
    }
};

// Parse assembly, store labels
unordered_map<string, int> label_map;
vector<pair<string, vector<string>>> instructions;

void parseAssembly(const string &filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Unable to open file " << filename << endl;
        exit(1);
    }

    string line;
    int index = 0;
    while (getline(file, line)) {
        size_t commentPos = line.find("#");
        if (commentPos != string::npos) line = line.substr(0, commentPos);
        if (line.empty()) continue;

        istringstream iss(line);
        string first, instr;
        iss >> first;

        if (first.back() == ':') {
            first.pop_back();
            label_map[first] = index;
        } else {
            instr = first;
            vector<string> args;
            string arg;
            while (getline(iss, arg, ',')) {
                while (!arg.empty() && (arg[0] == ' ' || arg[0] == '\t')) arg.erase(0, 1);
                args.push_back(arg);
            }
            instructions.emplace_back(instr, args);
            index++;
        }
    }
}

// Execute instructions
void executeProgram(Core &core) {
    while (core.pc < instructions.size()) {
        auto &instruction = instructions[core.pc];
        const string &instr = instruction.first;
        const vector<string> &args = instruction.second;
        if (core.visited_pcs.find(core.pc) != core.visited_pcs.end()) {
            cout << " Infinite loop detected at PC " << core.pc << " on core " << core.core_id << "\n";
            break;
        }
        core.visited_pcs.insert(core.pc);

        try {
            if (instr == "add") {
                core.registers[getRegisterIndex(args[0])] =
                    core.registers[getRegisterIndex(args[1])] + core.registers[getRegisterIndex(args[2])];
            } else if (instr == "sub") {
                core.registers[getRegisterIndex(args[0])] =
                    core.registers[getRegisterIndex(args[1])] - core.registers[getRegisterIndex(args[2])];
            } else if (instr == "bne") {
                if (core.registers[getRegisterIndex(args[0])] != core.registers[getRegisterIndex(args[1])]) {
                    core.pc = label_map[args[2]];
                    continue;
                }
            } else if (instr == "jal") {
                core.pc = label_map[args[1]];
                continue;
            } else if (instr == "lw") {
                core.registers[getRegisterIndex(args[0])] = core.lw(stoi(args[1]));
            } else if (instr == "sw") {
                core.sw(stoi(args[1]), core.registers[getRegisterIndex(args[0])]);
            } else if (instr == "addi") {
                core.registers[getRegisterIndex(args[0])] =
                    core.registers[getRegisterIndex(args[1])] + stoi(args[2]);
            } else {
                cout << "Unknown instruction: " << instr << endl;
            }
        } catch (exception &e) {
            cout << "Error processing instruction: " << instr << " -> " << e.what() << endl;
        }
        core.pc++;
    }
}

// Run simulator
void runSimulator(const string &filename) {
    vector<Core> cores;
    for (int i = 0; i < NUM_CORES; i++) cores.emplace_back(i);

    parseAssembly(filename);
    for (Core &core : cores) executeProgram(core);

    for (int i = 0; i < NUM_CORES; i++) {
        cout << "\nCore " << i << " Registers: ";
        for (int r = 0; r < REGISTERS; r++) cout << "x" << r << "=" << cores[i].registers[r] << " ";
        cout << endl;
    }
}

int main() {
    runSimulator("assembly.txt");
    return 0;
}
