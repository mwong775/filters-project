#include <bits/stdc++.h>
#include <random>
#include <unistd.h>
#include "hashutil.h"
#include "bf_cascade.h"
#include <time.h>
#include <string>
using std::string;
using std::cout;
using std::endl;

double time_cost(chrono::steady_clock::time_point &start,
                 chrono::steady_clock::time_point &end)
{
     /* Return the time elapse between start and end
      * count by seconds
      */
    double elapsedSeconds = ((end - start).count()) * chrono::steady_clock::period::num / static_cast<double>(chrono::steady_clock::period::den);
    return elapsedSeconds;
}

void test_bfc(vector<uint64_t> r, vector<uint64_t> s, vector<uint64_t> fp, FILE *file) {
    BFCascade<uint16_t, 15> bfc;
    fprintf(file, "Bloom Filter Cascade\n");
    fprintf(file, "level, insert, lookup, # fp's, fp, memory(bytes), bits per item\n");
    cout << "start bfc insert" << endl;
    auto start = chrono::steady_clock::now();  
    bfc.insert(r, s, fp); // , file
    auto end = chrono::steady_clock::now();
    cout << "finish bfc insert" << endl;
    double cost = time_cost(start, end);
    cout << "bfc insert time: " << cost << endl;
    fprintf(file, "bfc insert time\n");
    fprintf(file, "%f\n", cost);
    // cout << boolalpha; // converts bool print from 0/1 to t/f
    
    cout << "start bfc lookup" << endl;  
    start = chrono::steady_clock::now();  
    // returned true if REVOKED, false unrevoked
    int i = 0;
    int e1 = 0;
    for(auto c : r) { // int i = 0; i < int(r.size()); i++
        // uint64_t c = r.at(i);
        if(!bfc.lookup(c)) {
            cout << "BFC ERROR ON REVOKED " << c << " index " << i << endl; // should not print
            e1++;
            // break;
        }
        i++;
    }
    cout << "finish bfc revoked lookup " << i << endl;
    cout << "false neg's..? " << e1 << endl;
    int j = 0;
    int e2 = 0;
    for(auto d : s) {
        if(bfc.lookup(d)) {
            cout << "BFC ERROR ON UNREVOKED " << d << " index " << j << endl; // should not print
            e2++;
           // break;
        }
        j++;
    }
    end = chrono::steady_clock::now();
    cout << "finish bfc unrevoked lookup " << j << endl;
    cout << "false pos: " << e2 << endl;
    cost = time_cost(start, end);
    cout << "bfc lookup time: " << cost << endl;
    fprintf(file, "bf lookup time\n");
    fprintf(file, "%f\n", cost);
    // cout << bfc.lookup(s.at(0)) << '\n'; // unrevoked (lvl 1)
    // cout << bfc.lookup(r.at(0)) << '\n'; // revoked (lvl 2)
    // cout << bfc.lookup(s.at(43279)) << '\n'; // fp, unrevoked (lvl 2)
    bfc.print_cascade();
}

void read_cert(int f) {
    string line;
    string revoked_filename = "../final_revoked_unique.txt";
    string unrevoked_filename = "../final_unrevoked_unique.txt";
    if(f == 0) {
        revoked_filename = "../revoked_sorted.txt";
        unrevoked_filename = "../unrevoked_sorted.txt";
    }
    ifstream revoked(revoked_filename); // 500 , final_revoked.txt 27496
    ifstream unrevoked(unrevoked_filename); // 50000 , final_unrevoked.txt 29725064
    vector<uint64_t> r; // revoked 
    vector<uint64_t> s; // unrevoked
    vector<uint64_t> fp; // track false positives  

    FILE *file = fopen("bfc_test_fp.csv", "a");  // to print all stats to a file
    if (file == NULL)
    {
        perror("Couldn't open file\n");
        return;
    }

    auto start = chrono::steady_clock::now();
    while(getline(revoked, line)) { // gets revoked line by line, saves in string
        uint64_t i = stoul(line.c_str(), nullptr, 0); // convert string to long
        r.push_back(i);
    }
    revoked.close();

    while(getline(unrevoked, line)) { // gets unrevoked line by line
        uint64_t l = stoul(line.c_str(), nullptr, 0);
        s.push_back(l);
    }
    unrevoked.close();
    auto end = chrono::steady_clock::now();
    double cost = time_cost(start, end);
    cout << "time cost for read: " << cost << endl;

    fprintf(file, "revoked, unrevoked\n");
    fprintf(file, "%lu, %lu\n", r.size(), s.size());
    // cout << "# revoked: " << r.size() << " cap: " << r.capacity() << '\n';
    // cout << "# unrevoked: " << s.size() << " cap: " << s.capacity() << '\n';
    // cout << "# fp's: " << fp.size() << " cap: " << fp.capacity() << '\n';

    test_bfc(r, s, fp, file);

    fprintf(file, "\n");
    fclose(file);
}


int main(int argc, char *argv[]) {
    // 0 = short dataset, else full dataset
    read_cert(1); 

    /*
    mt19937 rd(123);
    __uint64_t x = rd();
    printf("mersenne twister test: %lu\n", x);
    */
    // vector<uint64_t> keys; // insert random keys here
    /*
    int n = 5;//(1 << 27) * 0.95; // 127506841
    printf("n: %d\n", n); // # items in the set expected for inserting
	BloomFilter<uint16_t, 15> bf;
    bf.init(n, 1);
    bf.print_filter();
    printf("\n");
    */
	return 0;
}

// n = 5, mem consumption = 20
// n = 2, mem consumption = 12