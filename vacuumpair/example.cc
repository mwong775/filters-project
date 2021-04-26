#include "vacuumfilter/vacuumfilter.h"
#include "vacuumhashtable/hashtable/vacuumhashtable.hh"
#include "vacuumhashtable/city_hasher.hh"

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
    cout << "init size: " << init_size << "\n";

    vacuumhashtable::vacuum_hashtable<KeyType, 12, CityHasher<KeyType>> table(init_size);

    // cuckoofilter::VacuumFilter<KeyType, 12, CityHasher<KeyType>> vf(init_size);

    // add set R to table
    for (KeyType c : r)
    {
        table.insert(c);
        // vf.Add(c);
    }

    // check for false negatives with set R
    // size_t false_negs = 0;
    for (KeyType c : r)
    {
        std::pair<int32_t, int32_t> indices = table.lookup(c);
        assert(indices.first >= 0 || indices.second >= 0);
        assert(table.find(c).first >= 0); // first = index, second = slot
    }

    size_t total_queries = 0;
    size_t false_queries = 0;
    size_t definite_queries = 0;

    for (KeyType l : s)
    {
        // if (vf.Contain(l) == cuckoofilter::Ok)
        // false_queries++;
        if (table.find(l).first >= 0) // first = index, second = slot
            definite_queries++;
        std::pair<int32_t, int32_t> indices = table.lookup(l);
        if (indices.first >= 0 || indices.second >= 0) // 1st index, 2nd index (NOTE: DIFFERENT FROM find())
        {
            if (indices.first >= 0)
                false_queries++;
            if (indices.second >= 0)
                false_queries++;
        }
        else
            assert(indices.first == -1 && indices.second == -1); // ensures zero/both buckets didn't yield false positives
        total_queries++;
    }
    assert(definite_queries == 0); // normal HT should only result in true negatives, no fp's

    double fp = (double)false_queries * 100.0 / total_queries;
    cout << "total false positives: " << false_queries << " out of " << total_queries
         << ", fp rate: " << fp << "%\n";

    cout << table.info() << "\n";
    // cout << vf.Info() << "\n";

    return 0;
}