#include <bits/stdc++.h>
#include <random>
#include <unistd.h>
#include "hashutil.h"
#include "cuckoo.h"

using namespace std;

// generate n 64-bit random numbers
void random_gen(int n, vector<uint64_t> &store, mt19937 &rd)
{
    store.resize(n); // uses Mersenne Twister pseudo-random generator of 32-bit numbers with a state size of 19937 bits..?
    // printf("generating dem random num nums\n");
    for (int i = 0; i < n; i++)
    {
        store[i] = (uint64_t(rd()) << 32) + rd();
        // printf("%lu\n", store[i]);
    }
}

int main(int argc, char **argv)
{
    typedef uint64_t KeyType;
    vector<KeyType> r;
    vector<KeyType> s;
    uint64_t size = 0;
    if (argc <= 1)
    {
        cout << "please enter an insertion set size D:\n";
        return (1);
    }
    size = atoi(argv[1]); // 240000 for ~91% load factor

    int seed = 1;
    mt19937 rd(seed);

    // 64-bit random numbers to insert and lookup -> lookup_size = insert_size * 100
    random_gen(size, r, rd);
    random_gen(size * 100, s, rd);

    // max load factor of 95%
    double max_lf = 0.95;
    uint64_t init_size = size / max_lf;

    BloomFilter<uint16_t, 15> bf;
    bf.init(init_size, 1);

    for (KeyType c : r)
    {
        bf.insert(c);
    }

    size_t total_queries = 0;
    size_t false_queries = 0;

    for (KeyType l : s) {
        if (bf.lookup(l))
            false_queries++;
        total_queries++;
    }

    double fp = (double)false_queries * 100.0 / total_queries;
    cout << "total false positives: " << false_queries << " out of " << total_queries
         << ", fp rate: " << fp << "%\n" << "memory cost: " << bf.mem_cost() << "\n";
}