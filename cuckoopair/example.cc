#include "cuckoofilter/src/cuckoofilter.h"

#include <math.h>
#include <cstdlib>

#include "cuckoohashtable/city_hasher.hh"
#include "cuckoohashtable/hashtable/cuckoohashtable.hh"
#include <time.h>

using namespace std;

/**
 * Initial test file for cuckoo hashing before creating header file
 * before abstracting the data structure pair and rehashing algorithm
*/

double time_cost(chrono::steady_clock::time_point &start,
                 chrono::steady_clock::time_point &end)
{
    /* Return the time elapse between start and end
      * count by seconds
      */
    double elapsedSeconds = ((end - start).count()) * chrono::steady_clock::period::num / static_cast<double>(chrono::steady_clock::period::den);
    return elapsedSeconds;
}

// generate n 64-bit random numbers for running insert & lookup
void random_gen(int n, vector<uint64_t> &store, mt19937 &rd)
{
    store.resize(n);
    for (int i = 0; i < n; i++)
        store[i] = (uint64_t(rd()) << 32) + rd();
}

template <typename KeyType>
vector<uint8_t> hashtable_ops(const uint64_t &init_size, vector<KeyType> &r, vector<KeyType> &s, vector<vector<KeyType>> &fp_table, FILE *file)
{
    cuckoohashtable::cuckoo_hashtable<KeyType, 12, CityHasher<KeyType>> table(init_size);

    // add set R to table
    for (KeyType c : r)
    {
        table.insert(c);
    }

    // check for false negatives with set R
    // size_t false_negs = 0;
    for (KeyType c : r)
    {
        std::pair<int32_t, int32_t> indices = table.lookup(c);
        assert(indices.first >= 0 || indices.second >= 0);
        assert(table.find(c).first >= 0); // first = index, second = slot
    }

    // lookup set S and count false positives

    // track buckets needing rehash using a set
    std::unordered_set<int32_t> rehashBSet;
    int total_rehash = 0;

    /**
     * We check for false positives by looking up fingerprints using
     * mutually exclusive set S. Buckets yielding false positives are rehashed
     * with an incremented seed until no false positives remain from lookup.
     */
    fprintf(file, "lookup round, false positives, percent fp's\n");
    while (1)
    {
        size_t total_queries = 0;
        size_t false_queries = 0;
        size_t definite_queries = 0;

        table.start_lookup();
        for (KeyType l : s)
        {
            if (table.find(l).first >= 0)
                definite_queries++;
            total_queries++;

            std::pair<int32_t, int32_t> indices = table.lookup(l);
            if (indices.first >= 0 || indices.second >= 0)
            {
                if (indices.first >= 0)
                {
                    false_queries++;
                    rehashBSet.insert(indices.first);
                }
                if (indices.second >= 0)
                {
                    false_queries++;
                    rehashBSet.insert(indices.second);
                }
            }
            else
                assert(indices.first == -1 && indices.second == -1); // ensures zero/both buckets didn't yield false positives
        }
        assert(definite_queries == 0); // normal HT should only result in true negatives, no fp's

        double fp = (double)false_queries * 100.0 / total_queries;
        cout << "total false positives: " << false_queries << " out of " << total_queries
             << ", fp rate: " << fp << "%\n";

        fprintf(file, "%lu, %lu, %.6f\n", table.num_rehashes() + 1, false_queries, fp);

        if (false_queries > 0)
            total_rehash += table.rehash_buckets();
        else
            break;
    }

    cout << table.info();
    fprintf(file, "\nslot per bucket, bucket count, capacity, load factor\n");
    fprintf(file, "%d, %lu, %lu, %.2f\n\n", table.slot_per_bucket(), table.bucket_count(), table.capacity(), table.load_factor() * 100.0);
    // table.bucketInfo();

    std::map<uint8_t, uint16_t> seed_map;
    cout << table.seedInfo(seed_map);
    fprintf(file, "rehashes per bucket, count\n");
    for (auto &k : seed_map)
    {
        fprintf(file, "%d, %d\n", k.first, k.second);
    }

    double avg_rehashes = (double)total_rehash / table.bucket_count();
    double rehash_percent = (double)rehashBSet.size() * 100.0 / table.bucket_count();
    fprintf(file, "\ntotal rehashes, max rehash, average per bucket, percent rehashed buckets\n");
    fprintf(file, "%d, %lu, %.4f, %.3f\n\n", total_rehash, table.num_rehashes(), avg_rehashes, rehash_percent);

    table.export_table(fp_table);

    // size: 10k, hashpower: 12, hashmask: 4095
    // cout << "HT hashpower: " << table.hashpower() << " hashmask: " << table.hashmask(table.hashpower()) << "\n";

    return table.get_seeds();
}

template <typename KeyType>
void create_filter(const uint64_t &init_size, vector<vector<KeyType>> &fp_table, vector<uint8_t> &seeds, vector<KeyType> &r, vector<KeyType> &s, FILE *file)
{
    cuckoofilter::CuckooFilter<KeyType, 12, CityHasher<KeyType>> filter(init_size, seeds);

    // add Set R to filter
    cout << "fp table size: " << fp_table.size() << "\n";

    for (int i = 0; i < fp_table.size(); i++)
    {
        vector<KeyType> &b = fp_table.at(i);
        // cout << "bucket size: " << b.size() << "\n";

        for (int j = 0; j < b.size(); j++)
        {
            assert(filter.CopyInsert(b.at(j), i, j) == cuckoofilter::Ok);
        }
    }

    // check no false negatives - failing here with sizes above 10k :(
    cout << "\nChecking CF false negatives:\n";
    for (auto c : r)
        assert(filter.Contain(c) == cuckoofilter::Ok);

    size_t total_queries = 0;
    size_t false_queries = 0;
    for (auto l : s)
    {
        if (filter.Contain(l) == cuckoofilter::Ok)
            false_queries++;
        total_queries++;
    }

    assert(false_queries == 0);

    // Output the measured false positive rate
    cout << "false positive rate is "
         << 100.0 * false_queries / total_queries << "%"
         << " (count: " << false_queries << ")\n#" << filter.Info() << "\n";
}

template <typename KeyType>
void read_cert(int f, vector<KeyType> &r, vector<KeyType> &s, FILE *file)
{
    string line;
    string revoked_filename = "final_revoked_unique.txt";
    string unrevoked_filename = "final_unrevoked_unique.txt";
    if (f == 0)
    {
        revoked_filename = "revoked_sorted.txt"; // WARNING: unusable - NOT mutually exclusive sets/contains some shared hashes :(
        unrevoked_filename = "unrevoked_sorted.txt";
    }
    ifstream revoked(revoked_filename);     // 500 , final_revoked.txt 27496
    ifstream unrevoked(unrevoked_filename); // 50000 , final_unrevoked.txt 29725064

    auto start = chrono::steady_clock::now();
    while (getline(revoked, line))
    {                                                 // gets revoked line by line, saves in string
        uint64_t i = stoul(line.c_str(), nullptr, 0); // convert string to long
        r.push_back(i);
    }
    revoked.close();

    while (getline(unrevoked, line))
    { // gets unrevoked line by line
        uint64_t l = stoul(line.c_str(), nullptr, 0);
        s.push_back(l);
    }
    unrevoked.close();
    auto end = chrono::steady_clock::now();
    double cost = time_cost(start, end);
    cout << "time cost for read: " << cost << "\trevoked: " << r.size() << "\tunrevoked: " << s.size() << "\n";

    fprintf(file, "revoked, unrevoked\n");
    fprintf(file, "%lu, %lu\n", r.size(), s.size());
}

/**
     * CityHash usage:
     * init/declare: CityHasher<int> ch;
     * cout << key << " , cityhash: " << ch.operator()(key, seed);
    */

int main(int argc, char **argv)
{

    typedef uint64_t KeyType;
    vector<KeyType> r;
    vector<KeyType> s;
    uint64_t size = 0;

    FILE *file = fopen("cuckoo_pair.csv", "a"); // to print all stats to a file
    if (file == NULL)
    {
        perror("Couldn't open file\n");
        return {};
    }

    if (argc <= 1)
    {
        read_cert(1, r, s, file);
        size = r.size();
    }
    else
    {
        size = atoi(argv[1]); // 240000 for ~91% load factor

        int seed = 1;
        mt19937 rd(seed);

        // 64-bit random numbers to insert and lookup -> lookup_size = insert_size * 100
        random_gen(size, r, rd);
        random_gen(size * 100, s, rd);
    }

    // max load factor of 95%
    double max_lf = 0.95;
    uint64_t init_size = size / max_lf;
    cout << "init size: " << init_size << "\n";

    fprintf(file, "insert size, lookup size, init size, max percent load factor\n");
    fprintf(file, "%lu, %lu, %lu, %.1f\n\n", r.size(), s.size(), init_size, max_lf * 100);

    vector<vector<KeyType>> fp_table;
    vector<uint8_t> seeds = hashtable_ops(init_size, r, s, fp_table, file);

    create_filter(init_size, fp_table, seeds, r, s, file);

    fclose(file);

    return 0;
}