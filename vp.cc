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
    // double cost = time_cost(start, end);
    // cout << "time cost for read: " << cost << "\trevoked: " << r.size() << "\tunrevoked: " << s.size() << "\n";

    // fprintf(file, "revoked, unrevoked\n");
    // fprintf(file, "%lu, %lu\n", r.size(), s.size());
}

void test_lookup(int n = 0, int q = 0, int rept = 1)
{

    FILE *out = fopen("vp_throughput_lookup.csv", "a");
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

    printf("vacuum pair lookup\n");
    for (int t = 0; t < rept; t++)
    {

        // insertions take both insert and lookup sets as input
        // to generate more cascade levels upon finding false positive's
        vacuumpair<uint64_t> vp(insKey, lupKey);


        // int i = 0;
        // int ins_cnt = 0;
        // int j = 0;
        vp.info();
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
        fprintf(out, "%.5f, %.5f, %.5f\n", mop[k] / cnt[k], mop1[k] / cnt1[k], mop2[k] / cnt2[k]); // time over count?
    fprintf(out, "\n");

}

int main(int argc, char **argv)
{
    int rept = 5;
    test_lookup(0, 0, rept);

    return 0;
}