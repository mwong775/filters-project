#include "vacuumfilter/vacuumfilter.h"

using namespace std;

/**
 * Initial test file for vacuumpair/testing before creating header file
 * while converting from cuckoopair
*/

// generate n 64-bit random numbers for running insert & lookup
void random_gen(int n, vector<uint64_t> &store, mt19937 &rd)
{
    store.resize(n);
    for (int i = 0; i < n; i++)
        store[i] = (uint64_t(rd()) << 32) + rd();
}

template <typename KeyType>
void create_filter(const uint64_t &init_size, vector<vector<KeyType>> &fp_table, vector<uint16_t> &seeds, vector<KeyType> &r, vector<KeyType> &s, FILE *file = nullptr)
{
}

int main(int argc, char **argv)
{
    typedef uint64_t KeyType;
    vector<KeyType> r;
    vector<KeyType> s;
    uint64_t size = 0;

    size = atoi(argv[1]); // 240000 for ~91% load factor

    int seed = 1;
    mt19937 rd(seed);

    // 64-bit random numbers to insert and lookup -> lookup_size = insert_size * 100
    random_gen(size, r, rd);
    random_gen(size * 100, s, rd);

    // max load factor of 95%
    double max_lf = 0.95;
    uint64_t init_size = size / max_lf;
    cout << "init size: " << init_size << "\n";

    cuckoofilter::VacuumFilter<KeyType, 12> vf(init_size);

    // add set R to table
    for (KeyType c : r)
    {
        vf.Add(c);
    }

    size_t total_queries = 0;
    size_t false_queries = 0;

    for (KeyType l : s)
    {
        if (vf.Contain(l) == cuckoofilter::Ok)
            false_queries++;
        total_queries++;
    }

    double fp = (double)false_queries * 100.0 / total_queries;
    cout << "total false positives: " << false_queries << " out of " << total_queries
         << ", fp rate: " << fp << "%\n";

    cout << vf.Info() << "\n";

    return 0;
}