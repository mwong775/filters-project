#include <math.h>
#include <cstdlib>

#include "vacuumpair.hh"
#include <time.h>

// generate n 64-bit random numbers for running insert & lookup
void random_gen(int n, vector<uint64_t> &store, mt19937 &rd)
{
    store.resize(n);
    for (int i = 0; i < n; i++)
        store[i] = (uint64_t(rd()) << 32) + rd();
}

template <typename KeyType>
void read_cert(int f, vector<KeyType> &r, vector<KeyType> &s, FILE *file)
{
    string line;
    string revoked_filename = "../final_revoked_unique.txt";
    string unrevoked_filename = "../final_unrevoked_unique.txt";
    if (f == 0)
    {
        revoked_filename = "../revoked_sorted.txt"; // WARNING: unusable - NOT mutually exclusive sets/contains some shared hashes :(
        unrevoked_filename = "../unrevoked_sorted.txt";
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
    // double cost = time_cost(start, end);
    // cout << "time cost for read: " << cost << "\trevoked: " << r.size() << "\tunrevoked: " << s.size() << "\n";

    // fprintf(file, "revoked, unrevoked\n");
    // fprintf(file, "%lu, %lu\n", r.size(), s.size());
}

int main(int argc, char **argv)
{
    typedef uint64_t KeyType;
    vector<KeyType> r;
    vector<KeyType> s;
    uint64_t size = 0;

    FILE *file = fopen("vacuum_pair.csv", "a"); // to print all stats to a file
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
    vacuumpair<KeyType> pair(r, s);

    auto filter = pair.get_filter(); // "auto" seems scrappy lol TODO: figure out types in template, e.g. bits_per_fp and Hash

    return 0;
}