#include <bits/stdc++.h>
#include <random>
#include <unistd.h>
#include "bf_cascade/hashutil.h"
#include "bf_cascade/bf_cascade.h"
#include <time.h>
#include <string>
// using std::string;
// using std::cout;
// using std::endl;

using namespace std;

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

void read_cert(vector<uint64_t> &r, vector<uint64_t> &s)
{
    string line;
    string revoked_filename = "final_revoked_unique.txt";
    string unrevoked_filename = "final_unrevoked_unique.txt";

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
    cout << "time cost for read: " << cost << endl;

    // fprintf(file, "revoked, unrevoked\n");
    // fprintf(file, "%lu, %lu\n", r.size(), s.size());
    // cout << "# revoked: " << r.size() << " cap: " << r.capacity() << '\n';
    // cout << "# unrevoked: " << s.size() << " cap: " << s.capacity() << '\n';
    // cout << "# fp's: " << fp.size() << " cap: " << fp.capacity() << '\n';
}

void test_size_lookup(int n = 0, int q = 0, int rept = 1)
{
    FILE *out = fopen("bfc_size_lookup.csv", "a");
    assert(out != NULL);

    if (n == 0)
        n = (1 << 27);
    if (q == 0)
        q = 10000000;
    int seed = 1;

    mt19937 rd(seed);
    double mop[6][20], mop1[6][20], bits_per_item[6][20];
    int cnt[6][20], cnt1[6][20], total_bytes[6][20];
    uint8_t lvls[6][20];

    memcle(mop);
    memcle(cnt);
    memcle(mop1);
    memcle(cnt1);
    memcle(lvls);
    memcle(total_bytes);
    memcle(bits_per_item);

    printf("bfc size lookup\n");
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
            vector<uint64_t> fp; // track false positives
            random_gen(lim, insKey, rd);
            random_gen(p, lupKey, rd);
            // for (; i < lim; i++)
            //     if (bfc.Add(insKey[i]) == cuckoofilter::Ok)
            //         ++ins_cnt;

            // insertions take both insert and lookup sets as input
            // to generate more cascade levels upon finding false positive's
            BFCascade<uint16_t, 15> bfc;

            // insertions take both insert and lookup sets as input
            // to generate more cascade levels upon finding false positive's
            printf("level, insert, lookup, # fp's, fp, memory(bytes), bits per item\n"); // temp. method of formatting stdout since insert() is recursive
            bfc.insert(insKey, lupKey, fp);

            cout << "total cascade size (bytes): " << bfc.num_bytes() << "\n";
            lvls[0][j] = bfc.num_levels();
            total_bytes[0][j] = bfc.num_bytes();
            bits_per_item[0][j] = bfc.bits_per_item();

            auto start = chrono::steady_clock::now();

            for (k = 0; k < p; k++)
                if (!bfc.lookup(lupKey[k]))
                    lookup_number++;

            // for (; k < p; k++)
            //     if (bfc.lookup(insKey[k]))
            //         lookup_number++;

            auto end = chrono::steady_clock::now();
            double cost = time_cost(start, end);

            mop[0][j] += double(p) / 1000000.0 / cost;
            cnt[0][j] += 1;

            start = chrono::steady_clock::now();

            int t = min(q, lim);
            for (int i = 0; i < t; i++)
                if (bfc.lookup(insKey[i]) == true)
                    lookup_number++;

            end = chrono::steady_clock::now();
            cost = time_cost(start, end);
            mop1[0][j] += double(t) / 1000000.0 / cost;
            cnt1[0][j] += 1;
            printf("%.5f\n", double(t) / 1000000.0 / cost);

            printf("lookup_number = %d\n", lookup_number);
        }
    }

    fprintf(out, "num items, bfc neg, bfc pos, num lvls, total size, bits per item, item numbers = %d, query number = %d\n", n, q);

    for (int j = 0; j < 19; j++)
    {
        fprintf(out, "%d, ", int((j + 1) * 0.05 * n));
        for (int k = 0; k < 1; k++)
            fprintf(out, "%.5f, %.5f, %d, %d, %.5f, ", mop[k][j] / cnt[k][j], mop1[k][j] / cnt1[k][j], lvls[k][j], total_bytes[k][j], bits_per_item[k][j]);
        fprintf(out, "\n");
    }

    fclose(out);
}

void test_cert_lookup(int n = 0, int q = 0, int rept = 1)
{

    FILE *out = fopen("bfc_cert_lookup.csv", "a");
    assert(out != NULL);

    vector<uint64_t> insKey; // revoked
    vector<uint64_t> lupKey; // unrevoked
    vector<uint64_t> fp;     // track false positives

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
    // double mop[6][20], mop1[6][20];
    // int cnt[6][20], cnt1[6][20];
    memcle(mop);
    memcle(cnt);
    memcle(mop1);
    memcle(cnt1);
    memcle(mop2);
    memcle(cnt2);

    // max load factor of 95%
    // double max_lf = 0.95;
    // uint64_t init_size = insKey.size() / max_lf;

    printf("bloom filter cascade cert lookup\n");
    for (int t = 0; t < rept; t++)
    {
        // BloomFilter<uint16_t, 15> bf;
        // bfc.init(init_size, 1);
        BFCascade<uint16_t, 15> bfc;

        // int i = 0;
        // int ins_cnt = 0;
        // int j = 0;

        // insertions take both insert and lookup sets as input
        // to generate more cascade levels upon finding false positive's
        printf("level, insert, lookup, # fp's, fp, memory(bytes), bits per item\n"); // temp. method of formatting stdout since insert() is recursive
        bfc.insert(insKey, lupKey, fp);

        cout << "total cascade size (bytes): " << bfc.num_bytes() << "\n";
        printf("insert done\n");
        auto start = chrono::steady_clock::now();

        int lookup_number = 0;
        // int p = min(q, lim);
        // int k = 0;
        // START lookup set "s" / valid certs
        for (int k = 0; k < q; k++) // neg_frac = 0.5
            if (bfc.lookup(lupKey[k]) == false)
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
            if (bfc.lookup(insKey[i]) == true)
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
            if (bfc.lookup(lupKey[k]) == false)
                lookup_number++;
        // set R
        for (int i = 0; i < n; i++)
            if (bfc.lookup(insKey[i]) == true)
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
        fprintf(out, "%.5f, %.5f, %.5f\n", mop[k] / cnt[k], mop1[k] / cnt1[k], mop2[k] / cnt2[k]); // time over count?
    fprintf(out, "\n");
}

int main(int argc, char *argv[])
{
    int rept = 1;
    test_size_lookup(1000000, 100000000, rept);
    // test_cert_lookup(0, 0, rept);

    return 0;
}

// n = 5, mem consumption = 20
// n = 2, mem consumption = 12