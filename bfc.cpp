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

void test_bfc(vector<uint64_t> r, vector<uint64_t> s, vector<uint64_t> fp, FILE *file)
{
    BFCascade<uint16_t, 15> bfc;
    fprintf(file, "Bloom Filter Cascade\n");
    fprintf(file, "level, insert, lookup, # fp's, fp, memory(bytes), bits per item\n");
    cout << "start bfc insert" << endl;
    auto start = chrono::steady_clock::now();
    bfc.insert(r, s, fp);
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
    for (auto c : r)
    { // int i = 0; i < int(r.size()); i++
        // uint64_t c = r.at(i);
        if (!bfc.lookup(c))
        {
            cout << "bfc ERROR ON REVOKED " << c << " index " << i << endl; // should not print
            e1++;
            // break;
        }
        i++;
    }
    cout << "finish bfc revoked lookup " << i << endl;
    cout << "false neg's..? " << e1 << endl;
    int j = 0;
    int e2 = 0;
    for (auto d : s)
    {
        if (bfc.lookup(d))
        {
            cout << "bfc ERROR ON UNREVOKED " << d << " index " << j << endl; // should not print
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
    fprintf(file, "bfc lookup time\n");
    fprintf(file, "%f\n", cost);
    // cout << bfc.lookup(s.at(0)) << '\n'; // unrevoked (lvl 1)
    // cout << bfc.lookup(r.at(0)) << '\n'; // revoked (lvl 2)
    // cout << bfc.lookup(s.at(43279)) << '\n'; // fp, unrevoked (lvl 2)
    bfc.print_cascade();
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

    // test_bfc(r, s, fp, file);

    // fprintf(file, "\n");
    // fclose(file);
}

void test_lookup(int n = 0, int q = 0, int rept = 1)
{

    FILE *out = fopen("bfc_throughput_lookup.csv", "a");
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

    printf("bloom filter cascade lookup\n");
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
        bfc.insert(insKey, lupKey, fp);
        // for (; i < lim; i++)
        //     if (bfc.insert(insKey[i]) == true) {
        //         ++ins_cnt;
        //     }
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
    int rept = 5;
    test_lookup(0, 0, rept);

    return 0;
}

// n = 5, mem consumption = 20
// n = 2, mem consumption = 12