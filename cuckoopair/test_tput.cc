#include <bits/stdc++.h>
#include <random>
#include <unistd.h>
#include <time.h>
#include <chrono>

#include "cuckoopair.hh"

#define debug(a) cerr << #a << " = " << a << ' '
#define deln(a) cerr << #a << " = " << a << endl
#define memcle(a) memset(a, 0, sizeof(a))

using namespace std;

double time_cost(chrono::steady_clock::time_point &start,
                 chrono::steady_clock::time_point &end)
{
    /* Return the time elapse between start and end
      * count by seconds
      */
    double elapsedSeconds = ((end - start).count()) * chrono::steady_clock::period::num / static_cast<double>(chrono::steady_clock::period::den);
    return elapsedSeconds;
}

// generate n 64-bit random numbers
void random_gen(int n, vector<uint64_t> &store, mt19937 &rd)
{
    store.resize(n);
    for (int i = 0; i < n; i++)
        store[i] = (uint64_t(rd()) << 32) + rd();
}

void test_ins(int n = 0, int q = 0, int rept = 1)
{
    FILE *out = fopen("cf_throughput_ins.csv", "a");
    assert(out != NULL);

    if (n == 0)
        n = (1 << 27);
    if (q == 0)
        q = 10000000;

    int seed = 1;

    mt19937 rd(seed);
    double mop[6][30], mop1[6][30];
    int cnt[6][30], cnt1[6][30];

    memcle(mop);
    memcle(cnt);
    memcle(mop1);
    memcle(cnt1);

    printf("cf insert\n");
    for (int t = 0; t < rept; t++)
    {
        typedef uint64_t KeyType;
        vector<KeyType> insKey;
        vector<KeyType> lupKey;
        random_gen(n, insKey, rd);
        random_gen(q, lupKey, rd);

        double max_lf = 0.95;
        uint64_t init_size = n / max_lf;

        cuckoopair<KeyType> pair(insKey, lupKey);

        auto filter = pair.get_filter();
    }
}

int main()
{
    int rept = 5;
    //test_load_factor_new();
    // test_fp(0, 0, rept);
    test_ins(0, 0, rept);
    //test_lookup(0, 0, rept);
    //test_del(0, 0, rept);
    return 0;
}
