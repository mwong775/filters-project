#ifndef VACUUM_PAIR_HH
#define VACUUM_PAIR_HH

#include "vacuumfilter/vacuumfilter.h"
#include "vacuumhashtable/hashtable/vacuumhashtable.hh"
#include <math.h>
#include <bits/stdc++.h>
#include "vacuumhashtable/city_hasher.hh"

using namespace std;

/*
idea: class for performing zeroing false positive algorithm, and returning cuckoo filter with seeds
*/

template <typename KeyType, size_t bits_per_fp = 12, class Hash = CityHasher<KeyType>>
class vacuumpair
{
private:
    size_t num_items_;
    size_t size_;

    std::vector<uint8_t> seeds_;

public:
    vacuumhashtable::vacuum_hashtable<KeyType, bits_per_fp, Hash> *table_; // , CityHasher<KeyType>
    cuckoofilter::VacuumFilter<KeyType, bits_per_fp, Hash> *filter_;

public:
    explicit vacuumpair(vector<KeyType> r, vector<KeyType> s) : num_items_(r.size())
    {
        size_ = r.size() / 0.95;
        table_ = new vacuumhashtable::vacuum_hashtable<KeyType, bits_per_fp, Hash>(size_);
        insert_hashtable(r);
        fn_lookup_hashtable(r);

        zero_fp_rehash(r, s);

        // table_->printAllBuckets();

        vector<vector<KeyType>> fp_table;
        table_->export_table(fp_table);

        table_->seedInfo();

        filter_ = new cuckoofilter::VacuumFilter<KeyType, bits_per_fp, Hash>(size_, table_->get_seeds());

        insert_filter(fp_table);

        check_lookup_filter(r, s);

        cout << "complete!\n";

        info();
    }

    // ~vacuumpair()
    // {
    //     delete table_;
    //     delete filter_;
    // }

    template <typename K>
    void insert_hashtable(vector<K> &r)
    {
        // add set R to table
        for (K c : r)
            table_->insert(c);
        cout << "Hashtable: finish inserting " << table_->size() << " items\n";
    }

    template <typename K>
    void fn_lookup_hashtable(vector<K> &r)
    {
        // check for false negatives with set R
        for (KeyType c : r)
        {
            std::pair<int32_t, int32_t> indices = table_->lookup(c);
            assert(indices.first >= 0 || indices.second >= 0);
            assert(table_->find(c).first >= 0); // first = index, second = slot
        }
    }

    template <typename K>
    void zero_fp_rehash(vector<K> &r, vector<K> &s)
    {
        int total_rehash = 0;

        while (1)
        {
            size_t total_queries = 0;
            size_t false_queries = 0;
            size_t definite_queries = 0;

            table_->start_lookup();
            for (K l : s)
            {
                if (table_->find(l).first >= 0)
                    definite_queries++;
                total_queries++;

                std::pair<int32_t, int32_t> indices = table_->lookup(l);
                if (indices.first >= 0 || indices.second >= 0)
                {
                    if (indices.first >= 0)
                    {
                        false_queries++;
                        // rehashBSet.insert(indices.first);
                    }
                    if (indices.second >= 0)
                    {
                        false_queries++;
                        // rehashBSet.insert(indices.second);
                    }
                }
                else
                    assert(indices.first == -1 && indices.second == -1); // ensures zero/both buckets didn't yield false positives
            }
            assert(definite_queries == 0); // normal HT should only result in true negatives, no fp's

            double fp = (double)false_queries * 100.0 / total_queries;
            cout << "total false positives: " << false_queries << " out of " << total_queries
                 << ", fp rate: " << fp << "%\n";

            // fprintf(file, "%lu, %lu, %.6f\n", table.num_rehashes() + 1, false_queries, fp);

            if (false_queries > 0)
                total_rehash += table_->rehash_buckets();
            else
                break;
        }
        cout << table_->info();
    }

    template <typename K>
    void insert_filter(vector<vector<K>> &fp_table)
    {
        for (int i = 0; i < fp_table.size(); i++)
        {
            vector<KeyType> &b = fp_table.at(i);
            // cout << "bucket size: " << b.size() << "\n";

            for (int j = 0; j < b.size(); j++)
                assert(filter_->CopyInsert(b.at(j), i, j) == cuckoofilter::Ok);
        }

        cout << "Filter: finish inserting " << table_->size() << " items\n";
    }

    template <typename K>
    void check_lookup_filter(vector<K> &r, vector<K> &s)
    {
        // check no false negatives - failing here with sizes above 10k :(
        cout << "\nChecking VF false negatives...\n";
        for (auto c : r)
            assert(filter_->Contain(c) == cuckoofilter::Ok);

        size_t total_queries = 0;
        size_t false_queries = 0;
        // checking false positives (should be 0 after rehashes)
        cout << "Now checking VF false positives...\n";
        for (auto l : s)
        {
            if (filter_->Contain(l) == cuckoofilter::Ok)
                false_queries++;
            total_queries++;
        }
        assert(false_queries == 0);
        // assert(total_queries == filter_->Size() * 100);
    }

    cuckoofilter::VacuumFilter<KeyType, bits_per_fp, Hash> get_filter()
    {
        return *filter_;
    }

    void info()
    {
        cout << table_->info();
        cout << filter_->Info() << "\n";
    }

    void hashInfo()
    {
        table_->hashInfo();
    }
    // hashtable_t table_;
    // filter_t filter_;
};

#endif // VACUUM_PAIR_HH