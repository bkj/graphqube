/*
    main.cpp
    
    TODO:
        The hasher probably has too many collisions
*/

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <queue>

#include <omp.h>

using namespace std;

// --
// Typedefs

namespace std {
    template <> struct hash<std::pair<int, int>> {
        inline size_t operator()(const std::pair<int, int> &v) const {
            std::hash<int> int_hasher;
            return int_hasher(v.first) ^ int_hasher(v.second);
        }
    };
    template <> struct hash<std::unordered_map<int, int>> {
        inline size_t operator()(const std::unordered_map<int, int> &v) const {
            std::hash<int> int_hasher;
            long int out = 0;
            for ( auto it = v.begin(); it != v.end(); ++it ) {
                out += (int_hasher(it->first) ^ int_hasher(it->second));
            }
            return out;
        }
    };
}

typedef unordered_set<pair<int,int>> edgeset;
typedef unordered_map<int, int> intint_map;

typedef vector<pair<int, float>> neighbors_type_entry;
typedef unordered_map<int, neighbors_type_entry> neighbors_type;
typedef unordered_map<int, neighbors_type> neighbors;

// --
// Structs

struct Row {
    int src;
    int trg;
    float weight;
    int src_type;
    int trg_type;
    string edge_type;
};

struct Reference {
    neighbors neibs;
    edgeset visited;
    int num_edges;
};

struct Query {
    edgeset edges;
    intint_map types;
    int num_edges;
};

struct Candidate {
    edgeset edges;
    intint_map nodes;
    float weight;
    int num_uncovered_edges;
};

auto cmp = [](Candidate* left, Candidate* right) { return left->weight > right->weight;};
typedef priority_queue<Candidate*, vector<Candidate*>, decltype(cmp)> candidate_heap;

// --
// Helpers

Query load_query(string filename) {
    
    Query query;
    
    int src, trg, src_type, trg_type;
    float weight;
    string edge_type;

    string line;
    ifstream infile(filename, ifstream::in);
    infile.ignore(9999, '\n');
    while(getline(infile, line)) {

        istringstream iss(line);
        iss >> src;
        iss >> trg;
        iss >> weight;
        iss >> src_type;
        iss >> trg_type;
        iss >> edge_type;
        
        query.edges.insert(pair<int,int>(src, trg));
        query.edges.insert(pair<int,int>(trg, src));
        query.types.emplace(src, src_type);
        query.types.emplace(trg, trg_type);
    }
    query.num_edges = query.edges.size() / 2;
    return query;
}

Reference load_edgelist(string filename) {
    Reference reference;
    reference.num_edges = 0;
    
    int src, trg, src_type, trg_type;
    float weight;
    string edge_type;

    string line;
    ifstream infile(filename, ifstream::in);
    infile.ignore(9999, '\n');
    while(getline(infile, line)) {
        reference.num_edges += 1;
        
        istringstream iss(line);
        iss >> src;
        iss >> trg;
        iss >> weight;
        iss >> src_type;
        iss >> trg_type;
        iss >> edge_type;
        
        // Index edges
        if(reference.neibs.find(src) == reference.neibs.end()) {
            neighbors_type_entry tmp_type_entry;
            neighbors_type tmp_type;
            
            tmp_type_entry.push_back(pair<int,float>(trg, weight));
            tmp_type.emplace(trg_type, tmp_type_entry);
            reference.neibs.emplace(src, tmp_type);
        } else {
            if(reference.neibs[src].find(trg_type) == reference.neibs[src].end()) {
                neighbors_type_entry tmp_type_entry;
                tmp_type_entry.push_back(pair<int,float>(trg, weight));
                reference.neibs[src].emplace(trg_type, tmp_type_entry);
            } else {
                reference.neibs[src][trg_type].push_back(pair<int,float>(trg, weight));
            }
        }
        
        // Index reverse edges
        if(reference.neibs.find(trg) == reference.neibs.end()) {
            neighbors_type_entry tmp_type_entry;
            neighbors_type tmp_type;
            
            tmp_type_entry.push_back(pair<int,float>(src, weight));
            tmp_type.emplace(src_type, tmp_type_entry);
            reference.neibs.emplace(trg, tmp_type);
        } else {
            if(reference.neibs[trg].find(src_type) == reference.neibs[trg].end()) {
                neighbors_type_entry tmp_type_entry;
                tmp_type_entry.push_back(pair<int,float>(src, weight));
                reference.neibs[trg].emplace(src_type, tmp_type_entry);
            } else {
                reference.neibs[trg][src_type].push_back(pair<int,float>(src, weight));
            }
        }
        
    }
    
    return reference;
}

vector<Candidate*> get_initial_candidates(Query& query, Row& row) {    
    vector<Candidate*> initial_cands;
    initial_cands.clear();
    for (auto &q_edge : query.edges) {
        if(q_edge.first < q_edge.second) {
            
            int q_src_type = query.types[q_edge.first];
            int q_trg_type = query.types[q_edge.second];

            if((q_src_type == row.src_type) & (q_trg_type == row.trg_type)) {
                Candidate* new_cand = new Candidate;
                
                new_cand->edges.insert(pair<int,int>(q_edge.first, q_edge.second));
                new_cand->edges.insert(pair<int,int>(q_edge.second, q_edge.first));
                                
                new_cand->nodes.insert(pair<int,int>(q_edge.first, row.src));
                new_cand->nodes.insert(pair<int,int>(q_edge.second, row.trg));

                new_cand->num_uncovered_edges = query.num_edges - 1;
                
                new_cand->weight = row.weight;
                
                initial_cands.push_back(new_cand);
            }

            if((q_src_type == row.trg_type) & (q_trg_type == row.src_type)) {
                Candidate* new_cand = new Candidate;
                
                new_cand->edges.insert(pair<int,int>(q_edge.first, q_edge.second));
                new_cand->edges.insert(pair<int,int>(q_edge.second, q_edge.first));
                
                new_cand->nodes.insert(pair<int,int>(q_edge.first, row.trg));
                new_cand->nodes.insert(pair<int,int>(q_edge.second, row.src));
                
                new_cand->num_uncovered_edges = query.num_edges - 1;
                
                new_cand->weight = row.weight;
                
                initial_cands.push_back(new_cand);
                return initial_cands;
            }
       }
    }
    return initial_cands;
}

vector<Candidate*> expand_candidate(Query& query, Reference& reference, \
        vector<Candidate*>& initial_cands, const float max_weight, float top_k_weight) {
    
    vector<Candidate*> new_cands;
    vector<Candidate*> out_cands;
    
    out_cands.clear();
    new_cands.clear();
    
    int verbose = 0;
    
    vector<Candidate*>* curr_cand_ptr;
    vector<Candidate*>* new_cand_ptr;
    
    curr_cand_ptr = &initial_cands;
    new_cand_ptr = &new_cands;
    
    while(true) {
        for(auto &cand : *curr_cand_ptr) {

            if(verbose) {
                cout << "entry: ";
                for(auto it = cand->nodes.cbegin(); it != cand->nodes.cend(); ++it) {
                    cout << "(" << it->first << ": " << it->second << ") ";
                }
                cout << " " << cand->num_uncovered_edges << endl;
            }
                  
            int r_src, q_src, q_trg, q_trg_type;
            
            int flag = 0;
            for(auto &q_edge : query.edges) {
                if(cand->edges.find(q_edge) == cand->edges.end()) {
                    if(cand->nodes.find(q_edge.first) != cand->nodes.end()) {
                        r_src = cand->nodes[q_edge.first];
                        q_src = q_edge.first;
                        q_trg = q_edge.second;
                        q_trg_type = query.types[q_trg];
                        flag = 1;
                        break;
                    }
                }
            }
            
            if(flag == 0) {
                cerr << "!!!" << endl;
                throw;
            }
            
            int q_trg_current, q_trg_new;
            if(reference.neibs[r_src].find(q_trg_type) != reference.neibs[r_src].end()) {
                
                if(cand->nodes.find(q_trg) != cand->nodes.end()) {
                    q_trg_current = cand->nodes[q_trg];
                    q_trg_new = 0;
                } else {
                    q_trg_current = -1;
                    q_trg_new = 1;
                }
                
                unordered_set<int> cand_nodes_values;
                for(auto &kv : cand->nodes) {
                    cand_nodes_values.insert(kv.second);
                }
                
                for(auto &r_trg_weight : reference.neibs[r_src][q_trg_type]) {
                    int r_trg = r_trg_weight.first;
                    float r_edge_weight = r_trg_weight.second;
                    
                    if((q_trg_new && (cand_nodes_values.find(r_trg) == cand_nodes_values.end())) || (q_trg_current == r_trg)) {
                        
                        bool not_visited;
                        #pragma omp critical
                        {
                            not_visited = (reference.visited.find(pair<int,int>(r_src, r_trg)) == reference.visited.end());    
                        }
                        
                        if(not_visited) {

                            int new_num_uncovered_edges = cand->num_uncovered_edges - 1;
                            float new_weight = cand->weight + r_edge_weight;
                            float upper_bound = new_weight + max_weight * new_num_uncovered_edges;
                            
                            if(upper_bound > top_k_weight) {
                                Candidate* new_cand = new Candidate;
                                
                                new_cand->edges = cand->edges;
                                new_cand->edges.insert(pair<int,int>(q_src, q_trg));
                                new_cand->edges.insert(pair<int,int>(q_trg, q_src));
                                
                                new_cand->nodes = cand->nodes;
                                new_cand->nodes.insert(pair<int,int>(q_trg, r_trg));
                                
                                new_cand->weight = new_weight;
                                new_cand->num_uncovered_edges = cand->num_uncovered_edges - 1;

                                if(verbose) {
                                    cout << "new_cand: ";
                                    for(auto it = new_cand->nodes.cbegin(); it != new_cand->nodes.cend(); ++it) {
                                        cout << "(" << it->first << ": " << it->second << ") ";
                                    }
                                    cout << " " << new_cand->num_uncovered_edges << endl;
                                }
                                
                                if(new_num_uncovered_edges == 0) {
                                    out_cands.push_back(new_cand);
                                } else {
                                    new_cand_ptr->push_back(new_cand);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if(new_cand_ptr->size() == 0) {
            if(verbose) { cout << "out_cands.size() " << out_cands.size() << endl; }
            return out_cands;
        } else {
            if(verbose) { cout << "new_cands.size() " << new_cand_ptr->size() << endl; }
            curr_cand_ptr->clear();
            vector<Candidate*>* tmp_cand_ptr = curr_cand_ptr;
            curr_cand_ptr = new_cand_ptr;
            new_cand_ptr = tmp_cand_ptr;
        }
    }
}

template<typename T> void print_queue(T& q) {
    for(int i = 0; i < 50; ++i) {cout << "--";}; cout << endl;
    float total_weight = 0.0;
    while(!q.empty()) {
        total_weight += q.top()->weight;
        
        Candidate* top = q.top();
        for ( auto it = top->nodes.begin(); it != top->nodes.end(); ++it ) {
            cout << it->first << ":" << it->second << "\t";
        }
        cout << "weight:" << top->weight << endl;
        q.pop();
    }
    for(int i = 0; i < 50; ++i) {cout << "--";};
    cout << endl << "total_weight: " << total_weight << endl;
}

int main(int argc, char **argv) {

    if(argc != 4) {
        cerr << "usage: main <query-path> <edge-path> <k>" << endl;
        cerr << "  query-path   path to query" << endl;
        cerr << "  edge-path    path to edgelist" << endl;
        cerr << "  k            number of hits" << endl;
        return 1;
    }
    
    string query_path(argv[1]);
    string edge_path(argv[2]);
    int K = stoi(argv[3]);

    // -- 
    // IO
    
    // Load query  
    Query query = load_query(query_path);
    int num_query_edges = query.edges.size();
    cerr << "num_query_edges=" << num_query_edges << endl;

    // Load edges
    Reference reference = load_edgelist(edge_path);
    cerr << "num_reference_edges=" << reference.num_edges << endl;
        
    candidate_heap top_k(cmp);
    unordered_set<unordered_map<int,int>> top_k_members;
    
    // --
    // Run
    
    string line;
    ifstream infile(edge_path, ifstream::in);
    
    int counter;
    counter = 0;

    double start = omp_get_wtime();
    
    #pragma omp parallel
    {
        #pragma omp master
        {
            while(true) { 
                getline(infile, line);
                istringstream iss(line);
                Row row = {};
                iss >> row.src;
                iss >> row.trg;
                iss >> row.weight;
                iss >> row.src_type;
                iss >> row.trg_type;
                iss >> row.edge_type;
                
                float top_k_weight;
                if(top_k.size() >= K) {
                    top_k_weight = top_k.top()->weight;    
                } else {
                    top_k_weight = -1.0;
                }
                
                if(row.weight * query.num_edges < top_k_weight) {
                    cerr << "row.weight * query.num_edges < top_k_weight -- breaking" << endl;
                    break;
                }
                
                #pragma omp task firstprivate(row) shared(query,reference,top_k)
                {
                    vector<Candidate*> initial_candidates, out_cands; 
                    initial_candidates = get_initial_candidates(query, row);
                    out_cands = expand_candidate(query, reference, initial_candidates, row.weight, top_k_weight);
                                        
                    for(auto &out_cand : out_cands) {
                        if(top_k_members.find(out_cand->nodes) == top_k_members.end()) {
                            if(top_k.size() < K) {
                                #pragma omp critical
                                {
                                    top_k.push(out_cand);
                                    top_k_members.insert(out_cand->nodes);                                
                                }
                            } else if (out_cand->weight > top_k.top()->weight) {
                                #pragma omp critical
                                {
                                    top_k.pop();
                                    top_k.push(out_cand);
                                    top_k_members.insert(out_cand->nodes);                                    
                                }
                            }
                        }
                    }
                    initial_candidates.clear();
                    out_cands.clear();
                    
                    #pragma omp critical
                    {
                        reference.visited.insert(pair<int,int>(row.src, row.trg));
                        reference.visited.insert(pair<int,int>(row.trg, row.src));
                    }
                }
                
                counter++;
                if(counter == reference.num_edges) {
                    break;
                }
            }
        }
    }
    
    print_queue(top_k);
    double duration = (omp_get_wtime() - start);
    cout << "counter: " << counter << endl;
    cout << "runtime: " << duration << endl;
}
