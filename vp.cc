#include <math.h>
#include <cstdlib>

#include "vacuumpair/vacuumpair.hh"
#include <time.h>

#define memcle(a) memset(a, 0, sizeof(a))

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
void read_cert(vector<KeyType> &r, vector<KeyType> &s)
{
    string line;
    string revoked_filename = "final_revoked_unique.txt";     // 28,341,276
    string unrevoked_filename = "final_unrevoked_unique.txt"; // 262,328

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
    // double cost = time_cost(start, end);
    // cout << "time cost for read: " << cost << "\trevoked: " << r.size() << "\tunrevoked: " << s.size() << "\n";

    // fprintf(file, "revoked, unrevoked\n");
    // fprintf(file, "%lu, %lu\n", r.size(), s.size());
}

void test_lf_lookup(int n = 0, int q = 0, int rept = 1)
{
    FILE *out = fopen("vp_lf_lookup.csv", "a");
    assert(out != NULL);

    if (n == 0)
        n = (1 << 27);
    if (q == 0)
        q = 10000000;
    int seed = 1;
    // double neg_frac = 0.5;

    mt19937 rd(seed);
    double mop[6][20], mop1[6][20], avg_rh[6][20];
    int cnt[6][20], cnt1[6][20], max_rh[6][20];
    memcle(mop);
    memcle(cnt);
    memcle(mop1);
    memcle(cnt1);
    memcle(max_rh);
    memcle(avg_rh);


    int table_size = 0;
    int seedtable_size = 0;
    double bits_per_item = 0;

    printf("vp load factor lookup\n");
    for (int t = 0; t < rept; t++)
    {
        int i = 0;
        int ins_cnt = 0;
        int j = 0;

        for (double r = 0.05; r <= 0.96; r += 0.05, j++)
        {
            printf("r = %.2f\n", r);
            int lim = int(n * r);
            vector<uint64_t> insKey;
            int lookup_number = 0;
            int p = min(q, lim) * 100;
            int k = 0;
            vector<uint64_t> lupKey;
            random_gen(lim, insKey, rd);
            random_gen(p, lupKey, rd); // p * neg_frac
            // for (; i < lim; i++)
            //     if (vp.Add(insKey[i]) == cuckoofilter::Ok)
            //         ++ins_cnt;

            // insertions take both insert and lookup sets as input
            // to generate more cascade levels upon finding false positive's
            vacuumpair<uint64_t> vp(n);

            vp.init(insKey, lupKey);

            max_rh[0][j] += vp.num_rehashes(); 
            avg_rh[0][j] += vp.avg_rehash(); 

            table_size = vp.table_size();
            seedtable_size = vp.seedtable_size();
            bits_per_item = vp.bits_per_item();

            auto start = chrono::steady_clock::now();

            // for (k = 0; k < p * neg_frac; k++)
            //     if (!vp.lookup(lupKey[k]))
            //         lookup_number++;

            for (; k < p; k++)
                // if (vp.lookup(insKey[k]))
                if (!vp.lookup(lupKey[k]))
                    lookup_number++;

            auto end = chrono::steady_clock::now();
            double cost = time_cost(start, end);

            mop[0][j] += double(p) / 1000000.0 / cost;
            cnt[0][j] += 1;

            start = chrono::steady_clock::now();

            int t = min(q, lim);
            for (int i = 0; i < t; i++)
                if (vp.lookup(insKey[i]))
                    lookup_number++;

            end = chrono::steady_clock::now();
            cost = time_cost(start, end);
            mop1[0][j] += double(t) / 1000000.0 / cost;
            cnt1[0][j] += 1;
            printf("%.5f\n", double(t) / 1000000.0 / cost);

            printf("lookup_number = %d\n", lookup_number);
        }
    }

    fprintf(out, "occupancy, vp neg, vp pos, max rehash, avg rehash, table mem = %d, seedtable mem = %d, total mem = %d, bits per item = %.2f, item numbers = %d, query number = %d\n", table_size, seedtable_size, table_size + seedtable_size, bits_per_item, n, q);

    for (int j = 0; j < 19; j++)
    {
        fprintf(out, "%.2f, ", (j + 1) * 0.05);
        for (int k = 0; k < 1; k++)
            fprintf(out, "%.5f, %.5f, %d, %.5f, ", mop[k][j] / cnt[k][j], mop1[k][j] / cnt1[k][j], max_rh[k][j], avg_rh[k][j]);
        fprintf(out, "\n");
    }

    fclose(out);
}

void test_size_lookup(int n = 0, int q = 0, int rept = 1)
{
    FILE *out = fopen("vp_size_lookup.csv", "a");
    assert(out != NULL);

    if (n == 0)
        n = (1 << 27);
    if (q == 0)
        q = 10000000;
    int seed = 1;

    mt19937 rd(seed);
    double mop[6][20], mop1[6][20], bits_per_item[6][20];
    int cnt[6][20], cnt1[6][20], table_bytes[6][20], seed_bytes[6][20];
    memcle(mop);
    memcle(cnt);
    memcle(mop1);
    memcle(cnt1);
    memcle(bits_per_item);
    memcle(table_bytes);
    memcle(seed_bytes);

    printf("vp lookup\n");
    for (int t = 0; t < rept; t++)
    {
        int i = 0;
        int ins_cnt = 0;
        int j = 0;

        for (double r = 0.05; r <= 0.96; r += 0.05, j++)
        {
            printf("r = %.2f\n", r);
            int lim = int(n * r);
            vector<uint64_t> insKey;
            int lookup_number = 0;
            int p = min(q, lim) * 100;
            int k = 0;
            vector<uint64_t> lupKey;
            random_gen(lim, insKey, rd);
            random_gen(p, lupKey, rd);
            // for (; i < lim; i++)
            //     if (vp.Add(insKey[i]) == cuckoofilter::Ok)
            //         ++ins_cnt;

            // insertions take both insert and lookup sets as input
            // to generate more cascade levels upon finding false positive's
            vacuumpair<uint64_t> vp(insKey.size());

            vp.init(insKey, lupKey);

            bits_per_item[0][j] += vp.bits_per_item();
            table_bytes[0][j] += vp.table_size();
            seed_bytes[0][j] += vp.seedtable_size();

            auto start = chrono::steady_clock::now();

            for (k = 0; k < p; k++)
                if (!vp.lookup(lupKey[k]))
                    lookup_number++;

            // for (; k < p; k++)
            //     if (vp.lookup(insKey[k]))
            //         lookup_number++;

            auto end = chrono::steady_clock::now();
            double cost = time_cost(start, end);

            mop[0][j] += double(p) / 1000000.0 / cost;
            cnt[0][j] += 1;

            start = chrono::steady_clock::now();

            int t = min(q, lim);
            for (int i = 0; i < t; i++)
                if (vp.lookup(insKey[i]))
                    lookup_number++;

            end = chrono::steady_clock::now();
            cost = time_cost(start, end);
            mop1[0][j] += double(t) / 1000000.0 / cost;
            cnt1[0][j] += 1;
            printf("%.5f\n", double(t) / 1000000.0 / cost);

            printf("lookup_number = %d\n", lookup_number);
        }
    }

    fprintf(out, "occupancy, vp neg, vp pos, table size, seed size, total size, item numbers = %d, query number = %d\n", n, q);

    for (int j = 0; j < 19; j++)
    {
        fprintf(out, "%.2f, ", (j + 1) * 0.05);
        for (int k = 0; k < 1; k++)
            fprintf(out, "%.5f, %.5f, %d, %d, %d, ", mop[k][j] / cnt[k][j], mop1[k][j] / cnt1[k][j], table_bytes[k][j], seed_bytes[k][j], table_bytes[k][j] + seed_bytes[k][j]);
        fprintf(out, "\n");
    }

    fclose(out);
}

void test_cert_lookup(int n = 0, int q = 0, int rept = 1)
{

    FILE *out = fopen("vp_cert_lookup.csv", "a");
    assert(out != NULL);

    vector<uint64_t> insKey; // revoked
    vector<uint64_t> lupKey; // unrevoked

    read_cert(insKey, lupKey);

    if (n == 0)
        n = insKey.size();
    // n = (1 << 27) * 0.95;
    if (q == 0)
        q = lupKey.size();
    // q = 10000000;
    // double neg_frac = 0.5; // what is this

    double mop[6], mop1[6], mop2[6];
    int cnt[6], cnt1[6], cnt2[6];

    memcle(mop);
    memcle(cnt);
    memcle(mop1);
    memcle(cnt1);
    memcle(mop2);
    memcle(cnt2);

    // max load factor of 95%
    // double max_lf = 0.95;
    // uint64_t init_size = insKey.size() / max_lf;

    printf("vacuum pair cert lookup\n");
    for (int t = 0; t < rept; t++)
    {

        // insertions take both insert and lookup sets as input
        // to generate more cascade levels upon finding false positive's
        vacuumpair<uint64_t> vp(insKey.size());

        vp.init(insKey, lupKey);

        // int i = 0;
        // int ins_cnt = 0;
        // int j = 0;
        printf("insert done\n");
        auto start = chrono::steady_clock::now();

        int lookup_number = 0;
        // int p = min(q, lim);
        // int k = 0;
        // START lookup set "s" / valid certs
        for (int k = 0; k < q; k++) // neg_frac = 0.5
            if (vp.lookup(lupKey[k]) == false)
            {
                lookup_number++;
                // printf("neg lookup: %d false/miss\n", lookup_number);
            }

        auto end = chrono::steady_clock::now();
        double cost = time_cost(start, end);

        mop[t] += double(q) / 1000000.0 / cost;
        cnt[t] += 1;
        printf("time: %.5f\n", double(q) / 1000000.0 / cost);
        printf("valid lookup count: %d\n", lookup_number); // double of the limit

        // START lookup set "r" / revoked certs
        start = chrono::steady_clock::now();
        lookup_number = 0;
        for (int i = 0; i < n; i++)
            if (vp.lookup(insKey[i]) == true)
                lookup_number++;

        end = chrono::steady_clock::now();
        cost = time_cost(start, end);

        mop1[t] += double(n) / 1000000.0 / cost;
        cnt1[t] += 1;
        printf("time: %.5f\n", double(n) / 1000000.0 / cost);
        printf("revoked lookup count: %d\n", lookup_number);

        // START lookup mixed sets "r" + "s" / revoked + valid certs
        start = chrono::steady_clock::now();
        lookup_number = 0;
        // set S
        for (int k = 0; k < q; k++) // neg_frac = 0.5
            if (vp.lookup(lupKey[k]) == false)
                lookup_number++;
        // set R
        for (int i = 0; i < n; i++)
            if (vp.lookup(insKey[i]) == true)
                lookup_number++;

        end = chrono::steady_clock::now();
        cost = time_cost(start, end);

        mop2[t] += double(n + q) / 1000000.0 / cost;
        cnt2[t] += 1;
        printf("time: %.5f\n", double(n + q) / 1000000.0 / cost);
        printf("total mixed lookup count: %d\n", lookup_number);
    }
    fprintf(out, "valid throughput, revoked throughput, mixed throughput, item numbers = %d, query number = %d\n", n, q);
    // fprintf(out, "occupancy, bloom neg, bloom pos, negative fraction = %.2f, item numbers = %d, query number = %d\n", neg_frac, n, q);

    for (int k = 0; k < rept; k++)
        fprintf(out, "%.5f, %.5f, %.5f\n", mop[k] / cnt[k], mop1[k] / cnt1[k], mop2[k] / cnt2[k]);
    fprintf(out, "\n");
}

int main(int argc, char **argv)
{
    int rept = 1;
    test_lf_lookup(0, 0, rept);
    // test_lookup(0, 0, rept);
    // test_cert_lookup(0, 0, rept);

    return 0;
}