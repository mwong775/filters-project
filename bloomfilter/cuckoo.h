#ifndef CUCKOO_H
#define CUCKOO_H

#include "hashutil.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <random>
using namespace std;

#define memcle(a) memset(a, 0, sizeof(a))
#define sqr(a) ((a) * (a))
#define debug(a) cerr << #a << " = " << a << ' '
#define deln(a) cerr << #a << " = " << a << endl
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ROUNDDOWN(a, b) ((a) - ((a) % (b)))
#define ROUNDUP(a, b) ROUNDDOWN((a) + (b - 1), b)

const int long_seg = 262144;

unsigned long lower_power_of_two(unsigned long x)
{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

inline int find_highest_bit(int v)
{
// tricks of bit 
// from http://graphics.stanford.edu/~seander/bithacks.html
    static const int MultiplyDeBruijnBitPosition[32] = 
    {
      0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
      8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };

    v |= v >> 1; // first round down to one less than a power of 2 
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    int r = MultiplyDeBruijnBitPosition[(uint32_t)(v * 0x07C4ACDDU) >> 27];
    return r;
}



template <typename fp_t, int fp_len>
class Filter
{
    public : 

    long long n; // number of buckets
    int m; // number of slots per bucket
    uint64_t memory_consumption;
    virtual void init(int _n, int _m, int _max_kick_steps) = 0;
	virtual void clear() = 0;
    virtual int insert(uint64_t ele) = 0;
    virtual bool lookup(uint64_t ele) = 0;
    virtual bool del(uint64_t ele) = 0;
    uint64_t position_hash(long long ele); // hash to range [0, n - 1]
    virtual double get_load_factor() {return 0;}
    virtual double get_full_bucket_factor() {return 0;}
    virtual void debug_test() {}
};

template <typename fp_t, int fp_len>
uint64_t Filter<fp_t, fp_len>::position_hash(long long ele)
{
    return (ele % n + n) % n;
}

template <typename fp_t, int fp_len>
class BloomFilter //: public Filter<fp_t, fp_len>
{
    public : 
    // copied from Filter class
    long long n; // number of buckets
    int m; // number of slots per bucket
    uint64_t memory_consumption;

    int shift;
    int k;
    int a[20];
    char *T;
	// ~BloomFilter() { free(T); }
    uint64_t position_hash(long long ele) {
        return (ele % n + n) % n;
    }

    void init(int _n, int _m, int l = 1, float _fp_len = 0) // fp_len = m/n
    {
        mt19937 rd(123);
        for (int i = 0; i < 20; i++) 
        {
            a[i] = rd(); // initializes random numbers for hashing
        }

        shift = 3;
        _n += _n & 1;

        // if (_fp_len > 100 || _fp_len == 0) _fp_len = 1.44269504089;
        if (l == 1) {
            _fp_len = 10.46890611;
            k = int(_fp_len * 0.69314718056); // number of hashes - set to minimize false positive rate. long thing = ln(2)
        }
        else
        {
            _fp_len = 1.44269504089;
            k = 1; 
        }
        n = (uint64_t)_n * _m * _fp_len; // regard n * m as number of keys
        memory_consumption = ROUNDUP((long long)n + 64, 8) / 8; // how many bytes
        // cout << "k HASHES = " << k << endl;
        cout << "INIT mem: " << memory_consumption << " n: " << n << " fp_len: " << _fp_len << endl;
        T = (char *)calloc(memory_consumption, sizeof(char)); // how many bytes
    }
    void clear()
    {
	    memset(T, 0, sizeof(char) * this -> memory_consumption);
    }
    void set_item(uint64_t pos)
    {
        T[pos >> shift] |= (1LL << (pos & ((1LL << shift) - 1)));
    }
    bool get_item(uint64_t pos)
    {
        return (T[pos >> shift] & (1LL << (pos & ((1LL << shift) - 1)))) > 0;
    }
    uint64_t hash64(uint64_t ele)
    {
        //return (uint64_t(HashUtil::MurmurHash32(ele)) << 32) + (uint64_t(HashUtil::MurmurHash32(ele ^ 0x9128211)));
        return HashUtil::MurmurHash64(ele);
    }
    int insert(uint64_t ele)
    {
        // printf("inserting %lu\n", ele);
        for (int i = 0; i < k; i++) // k = number of hashes?
        {
            uint64_t h = hash64(ele ^ a[i]); // a = random numbers used in hash
            set_item(position_hash(h));
        }
    
        /*
        uint64_t h1 = hash64(ele ^ a[1]);
        uint64_t h2 = hash64(ele ^ a[2]);
        uint64_t h3 = hash64(ele ^ a[3]);
        uint64_t h4 = hash64(ele ^ a[4]);
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = hash64(ele ^ a[6]);
        uint64_t h7 = hash64(ele ^ a[7]);
        uint64_t h8 = hash64(ele ^ a[8]);

        set_item(this->position_hash(h1));
        set_item(this->position_hash(h2));
        set_item(this->position_hash(h3));
        set_item(this->position_hash(h4));
        set_item(this->position_hash(h5));
        set_item(this->position_hash(h6));
        set_item(this->position_hash(h7));
        set_item(this->position_hash(h8));
        */
        return 0;
    }
    bool lookup(uint64_t ele)
    {
        // printf("looking for %lu\n", ele);
        for (int i = 0; i < k; i++)
        {
            uint64_t h = hash64(ele ^ a[i]);
            if (get_item(position_hash(h)) == false) {
                // printf("%lu isn't here...\n", ele);
                return false;
            }
        }
        // printf("found %lu! I think\n", ele);
        return true;
        
        /*
        uint64_t h1 = hash64(ele ^ a[1]);
        uint64_t h2 = hash64(ele ^ a[2]);
        uint64_t h3 = hash64(ele ^ a[3]);
        uint64_t h4 = hash64(ele ^ a[4]);
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = hash64(ele ^ a[6]);
        uint64_t h7 = hash64(ele ^ a[7]);
        uint64_t h8 = hash64(ele ^ a[8]);

        return get_item(this->position_hash(h1)) && get_item(this->position_hash(h2)) && get_item(this->position_hash(h3)) && get_item(this->position_hash(h4)) && get_item(this -> position_hash(h5)) && get_item(this->position_hash(h6)) && get_item(this->position_hash(h7)) && get_item(this->position_hash(h8));
        */
    }
    // lil' function to print out the filter, assuming it's smol :P
    void print_filter() {
        printf("printing filter\nmem consumption: %lu\n", this->memory_consumption);
        for(int i = 0; i < int(this->memory_consumption); i++) {
            printf("%d ", T[i]); // long() ?
        }
        printf("\n");
    }
    // retrieve memory cost (not sure what units tho :/)
    uint64_t mem_cost() {
        // sizeof(char) is 1 :0
        return sizeof(char) * this->memory_consumption;
    }

    double get_load_factor(){return 0;}
    double get_full_bucket_factor(){return 0;}
    void debug_test() {}
    bool del(uint64_t ele) {return 0;}
};

template <typename fp_t, int fp_len>
class BlockBloomFilter : public Filter<fp_t, fp_len>
{
    // here we use full cache line as one block
    public : 

    int shift;
    int block_size;
    int a[20];
    int k;
    char *T;
	~BlockBloomFilter() { free(T); }

    void init(int _n, int _m, int _fp_len = 0)
    {
        int _block_size = 512;
        mt19937 rd(123);
        for (int i = 0; i < 20; i++) 
        {
            a[i] = rd();
        }

        shift = 3;
        _n += _n & 1;

        if (_fp_len > 100 || _fp_len == 0) _fp_len = fp_len;
        this -> n = (uint64_t)_n * _m * _fp_len;
        this -> n = ROUNDUP(this -> n, _block_size);
        this -> memory_consumption = ROUNDUP((long long)this -> n + 64, 8) / 8;
        k = int(_fp_len * 0.69314718056);
        block_size = _block_size;
        T = (char *)calloc(this -> memory_consumption, sizeof(char)); // how many bytes
    }
    void clear()
    {
	    memset(T, 0, sizeof(char) * this -> memory_consumption);
    }
    void set_item(uint64_t pos)
    {
        T[pos >> shift] |= (1LL << (pos & ((1LL << shift) - 1)));
    }
    bool get_item(uint64_t pos)
    {
        return (T[pos >> shift] & (1LL << (pos & ((1LL << shift) - 1)))) > 0;
    }
    uint64_t hash64(uint64_t ele)
    {
        //return (uint64_t(HashUtil::MurmurHash32(ele)) << 32) + (uint64_t(HashUtil::MurmurHash32(ele ^ 0x9128211)));
        return HashUtil::MurmurHash64(ele);
    }
    int insert(uint64_t ele)
    {
        uint64_t h0 = hash64(ele ^ a[0]); // denote the block address
        uint64_t base = (h0 % (this -> n / block_size)) * block_size; // 512 bit --- one cache line !

        uint64_t h = 0;
        for (int i = 0; i < k; i++)
        {
            if (i % 7 == 0) 
                h = hash64(ele ^ a[i + 1]);
            uint64_t h1 = (h >> (9 * (i % 7))) & (block_size - 1);
            set_item(base + h1);
        }

        return 0;

        /*
        uint64_t h1 = hash64(ele ^ a[1]);
        uint64_t h2 = h0 >> 9;
        uint64_t h3 = h0 >> 18;
        uint64_t h4 = h0 >> 27;
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = h5 >> 9;
        uint64_t h7 = h5 >> 18;
        uint64_t h8 = h5 >> 27;


        set_item(base + (h1 & (block_size - 1)));
        set_item(base + (h2 & (block_size - 1)));
        set_item(base + (h3 & (block_size - 1)));
        set_item(base + (h4 & (block_size - 1)));
        set_item(base + (h5 & (block_size - 1)));
        set_item(base + (h6 & (block_size - 1)));
        set_item(base + (h7 & (block_size - 1)));
        set_item(base + (h8 & (block_size - 1)));
        return 0;
        */
    }
    bool lookup(uint64_t ele)
    {

        uint64_t h0 = hash64(ele ^ a[0]); // denote the block address
        uint64_t base = (h0 % (this -> n / block_size)) * block_size; // 512 bit --- one cache line !

        uint64_t h = 0;
        for (int i = 0; i < k; i++)
        {
            if (i % 7 == 0) 
                h = hash64(ele ^ a[i + 1]);
            uint64_t h1 = (h >> (9 * (i % 7))) & (block_size - 1);
            if (get_item(base + h1) == false)
                return false;
        }

        return true;

        /*
        uint64_t h0 = hash64(ele ^ a[0]); // denote the block address

        uint64_t h1 = hash64(ele ^ a[1]);
        uint64_t h2 = h0 >> 9;
        uint64_t h3 = h0 >> 18;
        uint64_t h4 = h0 >> 27;
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = h5 >> 9;
        uint64_t h7 = h5 >> 18;
        uint64_t h8 = h5 >> 27;

        uint64_t base = (h0 % (this -> n / block_size)) * block_size; // 512 bit --- one cache line !

        return get_item(base + (h1 & (block_size - 1)))
               && get_item(base + (h2 & (block_size - 1)))
               && get_item(base + (h3 & (block_size - 1)))
               && get_item(base + (h4 & (block_size - 1)))
               && get_item(base + (h5 & (block_size - 1)))
               && get_item(base + (h6 & (block_size - 1)))
               && get_item(base + (h7 & (block_size - 1)))
               && get_item(base + (h8 & (block_size - 1)));
        */
    }
    double get_load_factor(){return 0;}
    double get_full_bucket_factor(){return 0;}
    void debug_test() {}
    bool del(uint64_t ele) {return 0;}
};

template <typename fp_t, int fp_len>
class DeletableBloomFilter : public Filter<fp_t, fp_len>
{
    public : 

    int shift;
    int block_size;
    int a[16];
    int r; // the flag bit numbers
    int k;
    char *T;
	~DeletableBloomFilter() { free(T); }

    void init(int _n, int _m, int _fp_len)
    {
        return; // do nothing
    }
    void init(int _n, int _m, int _fp_len, int _block_size)
    {
        mt19937 rd(123);
        for (int i = 0; i < 16; i++) 
        {
            a[i] = rd();
        }

        shift = 3;
        _n += _n & 1;

        if (_fp_len > 100 || _fp_len == 0) _fp_len = fp_len;
        this -> n = (uint64_t)_n * _m * _fp_len;
        this -> n = ROUNDUP(this -> n, _block_size);
        this -> memory_consumption = ROUNDUP((long long)this -> n + 64, 8) / 8;
        block_size = _block_size;
        r = (this -> n) / (block_size + 1);
        k = int(fp_len * 0.69314718056);
        T = (char *)calloc(this -> memory_consumption, sizeof(char)); // how many bytes
    }
    void clear()
    {
	    memset(T, 0, sizeof(char) * this -> memory_consumption);
    }
    void set_flag(uint64_t pos)
    {
        T[pos >> shift] |= (1LL << (pos & ((1LL << shift) - 1)));
    }
    bool get_item(uint64_t pos)
    {
        return (T[pos >> shift] & (1LL << (pos & ((1LL << shift) - 1)))) > 0;
    }
    inline uint64_t get_block_number(uint64_t pos)
    {
        return (pos - r) / block_size;
    }
    void set_item(uint64_t pos)
    {
        if (get_item(pos) == true) 
            set_flag(get_block_number(pos));
        else
            T[pos >> shift] |= (1LL << (pos & ((1LL << shift) - 1)));
    }
    bool del_item(uint64_t pos)
    {
        if (get_item(get_block_number(pos)) == false)
        {
            T[pos >> shift] &= ~(1LL << (pos & ((1LL << shift) - 1)));
            return true; // success
        }
        else
            return false;
    }
    uint64_t hash64(uint64_t ele)
    {
        //return (uint64_t(HashUtil::MurmurHash32(ele)) << 32) + (uint64_t(HashUtil::MurmurHash32(ele ^ 0x9128211)));
        return HashUtil::MurmurHash64(ele);
    }
    uint64_t position_hash(uint64_t pos)
    {
        return pos % (this -> n - r) + r;
    }
    int insert(uint64_t ele)
    {
        for (int i = 0; i < k; i++)
        {
            uint64_t h = hash64(ele ^ a[i]);
            set_item(position_hash(h));
        }
        /*
        uint64_t h1 = hash64(ele ^ a[1]); 
        uint64_t h2 = hash64(ele ^ a[2]);
        uint64_t h3 = hash64(ele ^ a[3]);
        uint64_t h4 = hash64(ele ^ a[4]);
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = hash64(ele ^ a[6]);
        uint64_t h7 = hash64(ele ^ a[7]);
        uint64_t h8 = hash64(ele ^ a[8]);


        set_item(position_hash(h1));
        set_item(position_hash(h2));
        set_item(position_hash(h3));
        set_item(position_hash(h4));
        set_item(position_hash(h5));
        set_item(position_hash(h6));
        set_item(position_hash(h7));
        set_item(position_hash(h8));
        */
        return 0;
    }
    bool lookup(uint64_t ele)
    {
        for (int i = 0; i < k; i++)
        {
            uint64_t h = hash64(ele ^ a[i]);
            if (get_item(position_hash(h)) == false)
               return false;
        }
        return true;
        /*
        uint64_t h1 = hash64(ele ^ a[1]); 
        uint64_t h2 = hash64(ele ^ a[2]);
        uint64_t h3 = hash64(ele ^ a[3]);
        uint64_t h4 = hash64(ele ^ a[4]);
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = hash64(ele ^ a[6]);
        uint64_t h7 = hash64(ele ^ a[7]);
        uint64_t h8 = hash64(ele ^ a[8]);

        return get_item(position_hash(h1))
               && get_item(position_hash(h2))
               && get_item(position_hash(h3))
               && get_item(position_hash(h4))
               && get_item(position_hash(h5))
               && get_item(position_hash(h6))
               && get_item(position_hash(h7))
               && get_item(position_hash(h8));
            */
    }
    double get_load_factor(){return 0;}
    double get_full_bucket_factor(){return 0;}
    void debug_test() {}
    // when success, return 0
    bool del(uint64_t ele) 
    {
        bool success_del = true;
        for (int i = 0; i < k; i++)
        {
            uint64_t h = hash64(ele ^ a[i]);
            success_del &= del_item(position_hash(h));
        }

        return !success_del;
        /*
        uint64_t h1 = hash64(ele ^ a[1]); 
        uint64_t h2 = hash64(ele ^ a[2]);
        uint64_t h3 = hash64(ele ^ a[3]);
        uint64_t h4 = hash64(ele ^ a[4]);
        uint64_t h5 = hash64(ele ^ a[5]);
        uint64_t h6 = hash64(ele ^ a[6]);
        uint64_t h7 = hash64(ele ^ a[7]);
        uint64_t h8 = hash64(ele ^ a[8]);


        bool d1 = del_item(position_hash(h1));
        bool d2 = del_item(position_hash(h2));
        bool d3 = del_item(position_hash(h3));
        bool d4 = del_item(position_hash(h4));
        bool d5 = del_item(position_hash(h5));
        bool d6 = del_item(position_hash(h6));
        bool d7 = del_item(position_hash(h7));
        bool d8 = del_item(position_hash(h8));

        return !(d1 && d2 && d3 && d4 && d5 && d6 && d7 && d8); // if zero --- success!
        */
    }
};

/*
template <typename fp_t, int fp_len>
class CuckooFilter : public Filter<fp_t, fp_len>
{
    public : 

    int max_2_power;
    virtual void init(int _n, int _m, int _max_kick_steps);
	void clear();
    int insert(uint64_t ele);
    bool lookup(uint64_t ele);
    double get_load_factor();

    fp_t *T;
	~CuckooFilter() { free(T); }
    
    int filled_cell;
    int max_kick_steps;

    fp_t fingerprint(uint64_t ele); // 32-bit to 'fp_len'-bit fingerprint
    virtual fp_t get_item(int pos, int rk); // get cell value (one fingerprint) from tabel[pos][rk]
    virtual void set_item(int pos, int rk, fp_t fp); // set tabel[pos][rk] as fp

    virtual int alternate(int pos, fp_t fp) = 0; // get alternate position
    virtual int insert_to_bucket(int pos, fp_t fp); // insert one fingerprint to bucket [pos] 
    virtual int lookup_in_bucket(int pos, fp_t fp); // lookup one fingerprint in  bucket [pos]
    virtual void debug_test() {}
} ; 


template <typename fp_t, int fp_len>
void CuckooFilter<fp_t, fp_len>::init(int _n, int _m, int _step)
{
    this -> n = _n;
    this -> m = _m;
    this -> max_kick_steps = _step;
    this -> filled_cell = 0;
    this -> memory_consumption = int(_n * _m * (fp_len) * 1.0 / 8);

    max_2_power = 1;
    for (; max_2_power * 2 < _n; ) max_2_power <<= 1;

    this -> T = (fp_t*) calloc(this -> memory_consumption, sizeof(char));
}

template <typename fp_t, int fp_len>
void CuckooFilter<fp_t, fp_len>::clear()
{
	this -> filled_cell = 0;
	memset(this -> T, 0, this -> memory_consumption);
}

template <typename fp_t, int fp_len>
fp_t CuckooFilter<fp_t, fp_len>::fingerprint(uint64_t ele)
{
    fp_t h = HashUtil::BobHash32(&ele, 4) % ((1ull << fp_len) -1) + 1;
    return h;
}

template <typename fp_t, int fp_len>
fp_t CuckooFilter<fp_t, fp_len>::get_item(int pos, int rk)
{
    return (fp_t) this -> T[pos * this -> m + rk];
}

template <typename fp_t, int fp_len>
void CuckooFilter<fp_t, fp_len>::set_item(int pos, int rk, fp_t fp)
{
    this -> T[pos * this -> m + rk] = (fp_t) fp;
}

template <typename fp_t, int fp_len>
int CuckooFilter<fp_t, fp_len>::insert_to_bucket(int pos, fp_t fp)
{

    // if success return 0
    // if fail return 1

    for (int i = 0; i < this -> m; i++) 
        if (get_item(pos, i) == 0)
        {
            set_item(pos, i, fp);
            return 0;
        }

    return 1;
}

template <typename fp_t, int fp_len>
int CuckooFilter<fp_t, fp_len>::insert(uint64_t ele)
{

    // If insert success return 0
    // If insert fail return 1

    fp_t fp = fingerprint(ele);
    int cur1 = this -> position_hash(ele);
    int cur2 = alternate(cur1, fp);
    if (alternate(cur2, fp) != cur1)
    {
        deln(ele);
        deln(fp);
        printf("cur1 = %d alt cur1 = %d\n", cur1, alternate(cur1, fp));
        printf("cur2 = %d alt cur2 = %d\n", cur2, alternate(cur2, fp));
    }

    if (insert_to_bucket(cur1, fp) == 0) {filled_cell++; return 0;}
    if (insert_to_bucket(cur2, fp) == 0) {filled_cell++; return 0;}

    //randomly choose one bucket's elements to kick
	int cur = (rand() & 1) ? cur1 : cur2;
    int rk = rand() % this -> m;

    //get those item
    fp_t tmp_fp = get_item(cur, rk);
    set_item(cur, rk, fp);

    int alt = alternate(cur, tmp_fp);
    
    for (int i = 0; i < this -> max_kick_steps; i++)
    {
        if (insert_to_bucket(alt, tmp_fp) == 0) {filled_cell++; return 0;}
        rk = rand() % this -> m;
        fp = get_item(alt, rk);
        set_item(alt, rk, tmp_fp);

        tmp_fp = fp;
        alt = alternate(alt, tmp_fp);
    }

    return 1;
}

template <typename fp_t, int fp_len>
int CuckooFilter<fp_t, fp_len>::lookup_in_bucket(int pos, fp_t fp)
{
    // If lookup success return 1
    // If lookup fail and the bucket is full return 2
    // If lookup fail and the bucket is not full return 3

    int isFull = 1;
    for (int i = 0; i < this -> m; i++)
    {
        int t = get_item(pos, i);
        if (t == (fp_t) fp) 
            return 1;
        isFull &= (t != 0);
    }
    return (isFull) ? 2 : 3;
}

template <typename fp_t, int fp_len>
bool CuckooFilter<fp_t, fp_len>::lookup(uint64_t ele)
{

    // If ele is positive, return true
    // negative -- return false

	fp_t fp = fingerprint(ele);
    int pos1 = this -> position_hash(ele);
    int pos2 = alternate(pos1, fp);

    int ok1 = lookup_in_bucket(pos1, fp);

    if (ok1 == 1) return true;
    if (ok1 == 3) return false;

    int ok2 = lookup_in_bucket(pos2, fp);

    return ok2 == 1;
}

template <typename fp_t, int fp_len>
double CuckooFilter<fp_t, fp_len>::get_load_factor()
{
    return filled_cell * 1.0 / this -> n / this -> m;
}
*/

template <typename fp_t, int fp_len>
class SemiSortCuckooFilter : public Filter<fp_t, fp_len>
{
    public : 

    int max_2_power;
    int big_seg;
    int len[4];
    virtual void init(int _n, int _m, int _max_kick_steps);
	void clear();
    virtual int insert(uint64_t ele);
    bool lookup(uint64_t ele);
    virtual bool del(uint64_t ele);
    double get_load_factor();
    double get_full_bucket_factor();

    bool debug_flag = false;
    bool balance = true;
    uint32_t *T;
    uint32_t encode_table[1 << 16];
    uint32_t decode_table[1 << 16];

	~SemiSortCuckooFilter() { free(T); }
    
    int filled_cell;
    int full_bucket;
    int max_kick_steps;

    fp_t fingerprint(uint64_t ele); // 32-bit to 'fp_len'-bit fingerprint

    //interface for semi-sorted bucket
    void get_bucket(int pos, fp_t *store);
    void set_bucket(int pos, fp_t *sotre);
    void test_bucket();
    void make_balance();
    inline int high_bit(fp_t fp);
    inline int low_bit(fp_t fp);
    inline void sort_pair(fp_t &a, fp_t &b);

    virtual int alternate(int pos, fp_t fp) = 0; // get alternate position
    virtual int insert_to_bucket(fp_t *store, fp_t fp); // insert one fingerprint to bucket [pos] 
    virtual int lookup_in_bucket(int pos, fp_t fp); // lookup one fingerprint in  bucket [pos]
    virtual int del_in_bucket(int pos, fp_t fp); // lookup one fingerprint in  bucket [pos]
    void debug_test()
    {
        //debug_flag = true;
        //static int in_deg[1000][1000];
        //memcle(in_deg);

        /*
        mt19937 rd(123);
        for (int i = 1; i <= 400; i++)
        {
            int pos = this -> position_hash(rd());
            fp_t store[8];
            get_bucket(pos, store);

            int t = 0;
            for (int j = 0; j < this -> m; j++)
                t += store[j] != 0;
            if (t != this -> m)
                printf("pos = %d, cnt = %d\n", pos, t);
        }
        */

        /*
        static int in_deg[1000][1000];
        memset(in_deg, 1, sizeof(in_deg));
        for (int i = 0; i < this -> n; i++) in_deg[i][i] = 0;

        //printf("non-full index : \n");
        for (int i = 0; i < this -> n; i++)
        {
            fp_t store[8];
            get_bucket(i, store);
            int t = 0;
            for (int j = 0; j < this -> m; j++)
                if (store[j] != 0)
                {
                    int alt = this -> alternate(i, store[j]);
                    in_deg[i][alt] = 1;
                    t += store[j] != 0;
                }
            if (t != this -> m)
                printf("pos = %d, cnt = %d\n", i, t);
        }
        puts("");

        for (int k = 0; k < this -> n; k++)
            for (int i = 0; i < this -> n; i++)
                if (i != k)
                    for (int j = 0; j < this -> n; j++)
                        if (j != k && i != j)
                            in_deg[i][j] = MIN(in_deg[i][j], in_deg[i][k] + in_deg[k][j]);

            */

        /*

        for (int i = 0; i < this -> n; i++)
            for (int j = 0; j < (1 << fp_len); j++)
            {
                int t = this -> alternate(i, j);
                //printf("t = %d, i = %d\n", t, i);
                in_deg[i][t]++;
            }
        
        for (int i = 0; i < this -> n; i++)
        {
            for (int j = 0; j < this -> n; j++)
                printf("%d ", in_deg[i][j]);
            puts("");
        }
        */
    }
} ; 

int upperpower2(int x)
{
    int ret = 1;
    for (; ret * 2 < x; ) ret <<= 1;
    cout << "ret " << ret << endl;
    return ret;
}

// solve equation : 1 + x(logc - logx + 1) - c = 0
double F_d(double x, double c)
{
  return log(c) - log(x);
}
double F(double x, double c)
{
  return 1 + x * (log(c) - log(x) + 1) - c;
}
double solve_equation(double c) 
{
  double x = c + 0.1;
  while (abs(F(x, c)) > 0.01)
    x -= F(x, c) / F_d(x, c);
  return x;
}
double balls_in_bins_max_load(double balls, double bins)
{
    double m = balls;
    double n = bins;
    double c = m / (n * log(n));

    if (c < 5)
    {
      printf("c = %.5f\n", c);
      double dc = solve_equation(c);
      double ret = (dc - 1 + 2) * log(n);
      return ret;
    }

    double ret = (m / n) + 1.5 * sqrt(2 * m / n * log(n));
    //printf("m = %.2f, n = %.2f, m/n = %.5f, ret = %.5f\n", m, n, m/n, ret);
    return ret;
}

int proper_alt_range(int M, int i, int *len)
{
  //printf("%d %.2f\n", M, frac);
    double b = 4; // slots per bucket
    double lf = 0.95; // target load factor
    int alt_range = 8;
    for (; alt_range < M;)
    {
        double f = (4 - i) * 0.25;
        //for (int j = 0; j < i; j++)
        //    f += 0.25 * (alt_range * 1.0 / len[j]);
        //printf("f = %.5f\n", f);
      if (balls_in_bins_max_load(f * b * lf * M, M * 1.0 / alt_range) < 0.97 * b * alt_range)
        return alt_range;
      alt_range <<= 1;
    }
    return -1;
}

template <typename fp_t, int fp_len>
void SemiSortCuckooFilter<fp_t, fp_len>::init(int _n, int _m, int _step)
{
    _n += _n & 1;
    big_seg = 0;
    //big_seg = upperpower2(_n / 100);
    big_seg = max(big_seg, proper_alt_range(_n, 0, len));
    _n = ROUNDUP(_n, big_seg);

    big_seg--;
    len[0] = big_seg;
    for (int i = 1; i < 4; i++)
        len[i] = proper_alt_range(_n, i, len) - 1;

    len[0] = max(len[0], 1024);
    //len[3] = max(len[3], 31);
    len[3] = (len[3] + 1) * 2 - 1;

    printf("alt setting : ");
    for (int i = 0; i < 4; i++)
        printf("%d, ", len[i]);
    printf("\n");

    this -> n = _n;
    this -> m = _m;
    this -> max_kick_steps = _step;
    this -> filled_cell = 0;
    this -> full_bucket = 0;

    uint64_t how_many_bit = (uint64_t)this -> n * this -> m * (fp_len - 1);

    //deln(how_many_bit);

    this -> memory_consumption = ROUNDUP(how_many_bit + 64, 8) / 8 + 8; // how many bytes !

    max_2_power = 1;
    for (; max_2_power * 2 < _n; ) max_2_power <<= 1;

    //this -> t = (fp_t*) calloc(this -> n * this -> m, sizeof(fp_t));
    this -> T = (uint32_t *) calloc(this -> memory_consumption, sizeof(char));

    if (this -> m == 4)
    {
        int index = 0;
        /*
        for (int i = 0; i < 16; i++)
            for (int j = 0; j < ((i == 0) ? 1 : i); j++)
                for (int k = 0; k < ((j == 0) ? 1 : j); k++)
                    for (int l = 0; l < ((k == 0) ? 1 : k); l++)
                    {
                        int plain_bit = (i << 12) + (j << 8) + (k << 4) + l;
                        encode_table[plain_bit] = index;
                        decode_table[index] = plain_bit;
                        ++index;
                    }
        */
        for (int i = 0; i < 16; i++)
            for (int j = 0; j < ((i == 0) ? 1 : i + 1); j++)
                for (int k = 0; k < ((j == 0) ? 1 : j + 1); k++)
                    for (int l = 0; l < ((k == 0) ? 1 : k + 1); l++)
                    {
                        int plain_bit = (i << 12) + (j << 8) + (k << 4) + l;
                        encode_table[plain_bit] = index;
                        decode_table[index] = plain_bit;
                        ++index;
                    }
        //deln(index);
    }
}

template <typename fp_t, int fp_len>
void SemiSortCuckooFilter<fp_t, fp_len>::clear()
{
	this -> filled_cell = 0;
	//memset(this -> T, 0, sizeof(fp_t) * (this -> n * this -> m));
    memset(this -> T, 0, this -> memory_consumption);
}

template <typename fp_t, int fp_len>
fp_t SemiSortCuckooFilter<fp_t, fp_len>::fingerprint(uint64_t ele)
{
    //fp_t h = HashUtil::BobHash32(&ele, 4) % ((1ull << fp_len) -1) + 1;
    fp_t h = HashUtil::MurmurHash64(ele ^ 0x192837319273LL) % ((1ull << fp_len) -1) + 1;
    return h;
}


template <typename fp_t, int fp_len>
void SemiSortCuckooFilter<fp_t, fp_len>::get_bucket(int pos, fp_t *store)
{
    // Default : 
    //
    // Little Endian Store
    // Store by uint32_t
    // store[this -> m] = bucket number


    // 1. read the endcoded bits from memory

    int bucket_length = (fp_len - 1) * 4;
    uint64_t start_bit_pos = (uint64_t)pos * bucket_length;
    uint64_t end_bit_pos = start_bit_pos + bucket_length - 1;
    uint64_t result = 0;

    if (ROUNDDOWN(start_bit_pos, 64) == ROUNDDOWN(end_bit_pos, 64))
    {
        uint64_t unit = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
        //int reading_lower_bound = MAX(start_bit_pos, ROUNDDOWN(i, 64)) & 63;
        //int reading_upper_bound = MIN(end_bit_pos, ROUNDDOWN(i, 64) + 63) & 63;
        int reading_lower_bound = start_bit_pos & 63;
        int reading_upper_bound = end_bit_pos & 63;

        result = ((uint64_t)unit & ((-1ULL) >> (63 - reading_upper_bound))) >> reading_lower_bound;
    } else
    {
        //printf("%lu\n", ROUNDDOWN(start_bit_pos, 64) / 64);
        uint64_t unit1 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
        uint64_t unit2 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64 + 1];

        int reading_lower_bound = start_bit_pos & 63;
        int reading_upper_bound = end_bit_pos & 63;

        uint64_t t1 = unit1 >> reading_lower_bound;
        uint64_t t2 = (unit2 & ((1ULL << (reading_upper_bound + 1)) - 1)) << (64 - reading_lower_bound);
        result = t1 + t2;
    }

    /*
    for (int i = start_bit_pos; i < end_bit_pos; ) // i : bit position
    {

        uint64_t unit = ((uint64_t *)T)[ROUNDDOWN(i, 64) / 64];
        int reading_lower_bound = MAX(start_bit_pos, ROUNDDOWN(i, 64)) & 63;
        int reading_upper_bound = MIN(end_bit_pos, ROUNDDOWN(i, 64) + 63) & 63;
        uint64_t reading_result;

        reading_result = ((uint64_t)unit & ((-1ULL) >> (63 - reading_upper_bound))) >> reading_lower_bound;

        result = (reading_result << (i - start_bit_pos)) + result;
        //readed_bit_count += (reading_upper_bound - reading_lower_bound + 1);
        i += (reading_upper_bound - reading_lower_bound + 1);
    }
    */


    // 2. read the 4 elements from the encoded bits
    // We use 12 bits to store the 16 most significant bits for the items in bucket, 4 bits per item
    // the low bits are stored in the remaining bits
    //
    // For example, 8 bits per item , require 28 bits to store: 
    //
    // Original : 
    //
    // hhhh llll
    // hhhh llll
    // hhhh llll
    // hhhh llll
    //
    // encoded : 
    //
    //
    // 0 - 11                       12 - 15    16 - 19  20-23   24 - 27
    // HHHHHHHHHHHH                 llll       llll     llll    llll
    //  encoded high bit(12 bits)   item 0     item 1   item 2  item 3
    //

    /*
    for (int i = this -> m - 1; i >= 0; i--)
    {
        store[i] = result & ((1 << (fp_len - 4)) - 1);
        result >>= (fp_len - 4);
    }
    */

    int decode_result = decode_table[result >> (4 * (fp_len - 4))];

    store[3] = (result & ((1 << (fp_len - 4)) - 1)) + ((decode_result & ((1 << 4) - 1)) << (fp_len - 4));
    store[2] = ((result >> (1 * (fp_len - 4))) & ((1 << (fp_len - 4)) - 1)) + (((decode_result >> 4) & ((1 << 4) - 1)) << (fp_len - 4)); 
    store[1] = ((result >> (2 * (fp_len - 4))) & ((1 << (fp_len - 4)) - 1)) + (((decode_result >> 8) & ((1 << 4) - 1)) << (fp_len - 4));
    store[0] = ((result >> (3 * (fp_len - 4))) & ((1 << (fp_len - 4)) - 1)) + (((decode_result >> 12) & ((1 << 4) - 1)) << (fp_len - 4));

    store[4] = 0;
    store[4] += store[0] != 0;
    store[4] += store[1] != 0;
    store[4] += store[2] != 0;
    store[4] += store[3] != 0;
}

template <typename fp_t, int fp_len>
inline void SemiSortCuckooFilter<fp_t, fp_len>::sort_pair(fp_t &a, fp_t &b)
{
    if ((a) < (b)) swap(a, b);
}

template <typename fp_t, int fp_len>
void SemiSortCuckooFilter<fp_t, fp_len>::set_bucket(int pos, fp_t *store)
{
    // 0. sort store ! descendant order >>>>>>

    sort_pair(store[0], store[2]);
    sort_pair(store[1], store[3]);
    sort_pair(store[0], store[1]);
    sort_pair(store[2], store[3]);
    sort_pair(store[1], store[2]);

    /*
    for (int i = 0; i < this -> m - 1; i++)
        if (store[i] != 0 && store[i + 1] == store[i])
            printf("same ? ");
    */
    // 1. compute the encode 

    uint64_t high_bit = 0;
    uint64_t low_bit = 0;

    low_bit = (store[3] & ((1 << (fp_len - 4)) - 1)) 
            + ((store[2] & ((1 << (fp_len - 4)) - 1)) << (1 * (fp_len - 4)))
            + (((uint64_t)store[1] & ((1 << (fp_len - 4)) - 1)) << (2 * (fp_len - 4)))
            + (((uint64_t)store[0] & ((1 << (fp_len - 4)) - 1)) << (3 * (fp_len - 4)));

    high_bit = ((store[3] >> (fp_len - 4)) & ((1 << 4) - 1))
             + (((store[2] >> (fp_len - 4)) & ((1 << 4) - 1)) << 4)
             + (((store[1] >> (fp_len - 4)) & ((1 << 4) - 1)) << 8)
             + (((store[0] >> (fp_len - 4)) & ((1 << 4) - 1)) << 12);

             /*
    for (int i = 0; i < 4; i++)
    {
        low_bit = (low_bit << (fp_len - 4)) + (store[i] & ((1 << (fp_len - 4)) - 1));
        high_bit = (high_bit << 4) + ((store[i] >> (fp_len - 4)) & ((1 << 4) - 1));
    }
    */

    // 2. store into memory
    uint64_t high_encode = encode_table[high_bit];
    //printf("high_bit = %x\n", high_bit);
    //printf("high_encode = %x\n", high_encode);
    uint64_t all_encode = (high_encode << (4 * (fp_len - 4))) + low_bit;

    int bucket_length = (fp_len - 1) * 4;
    uint64_t start_bit_pos = (uint64_t)pos * bucket_length;
    uint64_t end_bit_pos = start_bit_pos + bucket_length - 1;

    if (ROUNDDOWN(start_bit_pos, 64) == ROUNDDOWN(end_bit_pos, 64))
    {
        uint64_t unit = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
        int writing_lower_bound = start_bit_pos & 63;
        int writing_upper_bound = end_bit_pos & 63;

        ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64] = (unit & (((1ULL << writing_lower_bound) - 1) + ((-1ULL) - ((-1ULL) >> (63 - writing_upper_bound)))))
                                               + ((all_encode & ((1ULL << (writing_upper_bound - writing_lower_bound + 1)) - 1)) << writing_lower_bound);
    } else
    {
        uint64_t unit1 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
        uint64_t unit2 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64 + 1];
        int writing_lower_bound = start_bit_pos & 63;
        int writing_upper_bound = end_bit_pos & 63;
        uint64_t lower_part = all_encode & ((1LL << (64 - writing_lower_bound)) - 1);
        uint64_t higher_part = all_encode >> (64 - writing_lower_bound);
        ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64] = (unit1 & ((1LL << writing_lower_bound) - 1)) 
                                                           + (lower_part << writing_lower_bound);
        ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64 + 1] = ((unit2 >> (writing_upper_bound + 1)) << (writing_upper_bound + 1))
                                                               + higher_part;
    }

    /*
    for (int i = start_bit_pos; i < end_bit_pos; ) // i : bit position
    {
        // in this 32-bit unit : 
        uint64_t unit = ((uint64_t *)T)[ROUNDDOWN(i, 64) / 64];
        int writing_lower_bound = MAX(start_bit_pos, ROUNDDOWN(i, 64)) & 63;
        int writing_upper_bound = MIN(end_bit_pos, ROUNDDOWN(i, 64) + 63) & 63;

        ((uint64_t *)T)[ROUNDDOWN(i, 64) / 64] = (unit & (((1ULL << writing_lower_bound) - 1) + ((-1ULL) - ((-1ULL) >> (63 - writing_upper_bound)))))
                                               + ((all_encode & ((1ULL << (writing_upper_bound - writing_lower_bound + 1)) - 1)) << writing_lower_bound);
        i += (writing_upper_bound - writing_lower_bound + 1);
        all_encode >>= (writing_upper_bound - writing_lower_bound + 1);
    }
    */
}

/*
template <typename fp_t, int fp_len>
fp_t CuckooFilter<fp_t, fp_len>::get_item(int pos, int rk)
{
    return (fp_t) this -> T[pos * this -> m + rk];
}

template <typename fp_t, int fp_len>
void CuckooFilter<fp_t, fp_len>::set_item(int pos, int rk, fp_t fp)
{
    this -> T[pos * this -> m + rk] = (fp_t) fp;
}
*/

template <typename fp_t, int fp_len>
inline int SemiSortCuckooFilter<fp_t, fp_len>::high_bit(fp_t fp)
{
    return (fp >> (fp_len - 4)) & ((1 << 4) - 1);
}

template <typename fp_t, int fp_len>
inline int SemiSortCuckooFilter<fp_t, fp_len>::low_bit(fp_t fp)
{
    return fp & ((1 << (fp_len - 4)) - 1);
}

template <typename fp_t, int fp_len>
int SemiSortCuckooFilter<fp_t, fp_len>::insert_to_bucket(fp_t *store, fp_t fp)
{

    // if success return 0
    // if find the same : return 1 + position
    // if full : return 1 + 4

    //get_bucket(pos, store);

    if (store[this -> m] == this -> m) 
        return 1 + 4;
    else 
    {
        store[3] = fp;
        return 0;
    }

    /*
    for (int i = 0; i < this -> m; i++) 
    {
        if (store[i] == 0)
        {
            if (i == this -> m - 1) // full
                full_bucket++;
            store[i] = fp;
            //set_bucket(pos, store);
            return 0;
        } 
        else
        // optimization for one bit
        if (high_bit(store[i]) == high_bit(fp))
            return 1 + i;
    }
    */

    //return 1 + 4;
}

template <typename fp_t, int fp_len>
int SemiSortCuckooFilter<fp_t, fp_len>::insert(uint64_t ele)
{
    return 1;
}

/*
template <typename fp_t, int fp_len>
void SemiSortCuckooFilter<fp_t, fp_len>::make_balance()
{
    int success = 0;
    for (int i = 0; i < this -> n; i++)
    {
        fp_t store[8], tmp[8];
        get_bucket(i, store);
        if (store[this -> m - 1] != 0) // full
        {
            for (int j = 0; j < this -> m; j++)
            {
                int alt = this -> alternate(i, store[j]);
                get_bucket(alt, tmp);
                if (tmp[this -> m - 2] == 0) // not full, <= 2
                {
                    tmp[this -> m - 2] = store[j];
                    set_bucket(alt, tmp);
                    store[j] = 0;
                    set_bucket(i, store);
                    full_bucket--;
                    success++;
                    break;
                }
            }
        }
    }
    deln(success);
}
*/

template <typename fp_t, int fp_len>
int SemiSortCuckooFilter<fp_t, fp_len>::lookup_in_bucket(int pos, fp_t fp)
{
    // If lookup success return 1
    // If lookup fail and the bucket is full return 2
    // If lookup fail and the bucket is not full return 3

    fp_t store[8];
    get_bucket(pos, store);

    int isFull = 1;
    for (int i = 0; i < this -> m; i++)
    {
        fp_t t = store[i];
        if (t == fp) 
            return 1;
        isFull &= (t != 0);
    }
    return (isFull) ? 2 : 3;
}

template <typename fp_t, int fp_len>
bool SemiSortCuckooFilter<fp_t, fp_len>::lookup(uint64_t ele)
{

    // If ele is positive, return true
    // negative -- return false

    ele = HashUtil::MurmurHash64(ele ^ 0x12891927);

	fp_t fp = fingerprint(ele);
    int pos1 = this -> position_hash(ele);

    int ok1 = lookup_in_bucket(pos1, fp);

    if (ok1 == 1) return true;
    //if (ok1 == 3) return false;

    int pos2 = alternate(pos1, fp);
    assert(pos1 == alternate(pos2, fp));
    int ok2 = lookup_in_bucket(pos2, fp);

    return ok2 == 1;
}

template <typename fp_t, int fp_len>
int SemiSortCuckooFilter<fp_t, fp_len>::del_in_bucket(int pos, fp_t fp)
{
    fp_t store[8];
    get_bucket(pos, store);

    for (int i = 0; i < this -> m; i++)
    {
        fp_t t = store[i];
        if (t == fp) 
        {
            store[i] = 0;
            --this -> filled_cell;
            set_bucket(pos, store);
            return 1;
        }
    }
    return 0;
}

template <typename fp_t, int fp_len>
bool SemiSortCuckooFilter<fp_t, fp_len>::del(uint64_t ele)
{

    // If ele is positive, return true
    // negative -- return false

    ele = HashUtil::MurmurHash64(ele ^ 0x12891927);
	fp_t fp = fingerprint(ele);
    int pos1 = this -> position_hash(ele);

    int ok1 = del_in_bucket(pos1, fp);

    if (ok1 == 1) return true;
    //if (ok1 == 3) return false;

    int pos2 = alternate(pos1, fp);
    int ok2 = del_in_bucket(pos2, fp);

    return ok2 == 1;
}
template <typename fp_t, int fp_len>
double SemiSortCuckooFilter<fp_t, fp_len>::get_load_factor()
{
    return filled_cell * 1.0 / this -> n / this -> m;
}

template <typename fp_t, int fp_len>
double SemiSortCuckooFilter<fp_t, fp_len>::get_full_bucket_factor()
{
    return full_bucket * 1.0 / this -> n;
}

template <typename fp_t, int fp_len>
void SemiSortCuckooFilter<fp_t, fp_len>::test_bucket()
{
    for (int i = 0; i < this -> n; i++)
    {
        //deln(i);
        fp_t store[8];
        store[0] = rand() % (1 << fp_len);
        store[1] = rand() % (1 << fp_len);
        store[2] = rand() % (1 << fp_len);
        store[3] = rand() % (1 << fp_len);

        for (int j = 0; j < this -> m; j++)
            for (int k = j + 1; k < this -> m; k++)
                if (store[j] == store[k])
                    store[k] = 0;

        set_bucket(i, store);

        fp_t tmp_store[8];
        get_bucket(i, tmp_store);
        for (int j = 0; j < 4; j++)
        {
            if (tmp_store[j] != store[j])
                printf("first i = %d, j = %d\n", i, j);
            assert(tmp_store[j] == store[j]);
        }
    }

    for (int i = 0; i < this -> n; i++)
    {
        fp_t store[8];
        store[0] = rand() % (1 << fp_len);
        store[1] = rand() % (1 << fp_len);
        store[2] = rand() % (1 << fp_len);
        store[3] = rand() % (1 << fp_len);

        for (int j = 0; j < this -> m; j++)
            for (int k = j + 1; k < this -> m; k++)
                if (store[j] == store[k])
                    store[k] = 0;

        set_bucket(i, store);

        fp_t tmp_store[8];
        get_bucket(i, tmp_store);
        for (int j = 0; j < 4; j++)
        {
            if (tmp_store[j] != store[j])
                printf("i = %d, j = %d\n", i, j);
            assert(tmp_store[j] == store[j]);
        }
    }
}

template <typename fp_t, int fp_len>
class MortonFilter : public Filter<fp_t, fp_len>
{
    public : 

    int max_2_power;
    virtual void init(int _n, int _m, int _max_kick_steps);
	void clear();
    int insert(uint64_t ele);
    bool lookup(uint64_t ele);
    bool del(uint64_t ele) {return 0;}
    double get_load_factor();

    uint8_t *T;

	~MortonFilter() { free(T); }
    
    int filled_cell;
    int max_kick_steps;
    int block_number;

    fp_t fingerprint(uint64_t ele); // 32-bit to 'fp_len'-bit fingerprint

    //interface for semi-sorted bucket
    void get_bucket(int pos, fp_t *store);
    void set_bucket(int pos, fp_t *sotre);
    void test_bucket();
    void print_block(int pos);
    int get_count(int block, int index);
    void set_count(int block, int index, int count);
    void block_kick_one(int pos, int &alt, fp_t &p);

    virtual int alternate(int pos, fp_t fp) = 0; // get alternate position
    virtual int insert_to_bucket(fp_t *store, fp_t fp); // insert one fingerprint to bucket [pos] 
    virtual int lookup_in_bucket(int pos, fp_t fp); // lookup one fingerprint in  bucket [pos]
} ; 


template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::init(int _n, int _m, int _step)
{
    _n += _n & 1;
    this -> n = _n;
    this -> m = _m;
    this -> max_kick_steps = _step;
    this -> filled_cell = 0;

    //deln(_n);
    //deln(_m);
    this -> block_number = ROUNDUP((uint64_t)_n * _m, 46) / 46;
    //deln(this -> block_number * 46);

    uint64_t how_many_bit = (uint64_t)this -> block_number * 512;
    //deln(how_many_bit);

    // here, we use memory_consumption as standard


    this -> memory_consumption = ROUNDUP(how_many_bit, 512) / 8; // how many bytes !
    this -> T = (uint8_t *) calloc(this -> memory_consumption, sizeof(char));

    this -> n = 64 * this -> block_number; // logical position number
    this -> m = 3; // m items perm bucket

    max_2_power = 1;

    for (; max_2_power * 2 < this -> n; ) max_2_power <<= 1;
}

template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::clear()
{
	this -> filled_cell = 0;
	//memset(this -> T, 0, sizeof(fp_t) * (this -> n * this -> m));
    memset(this -> T, 0, this -> memory_consumption);
}

template <typename fp_t, int fp_len>
fp_t MortonFilter<fp_t, fp_len>::fingerprint(uint64_t ele)
{
    //fp_t h = HashUtil::BobHash32(&ele, 4) % ((1ull << fp_len) -1) + 1;
    fp_t h = HashUtil::MurmurHash64(ele ^ 0x192837319273LL) % ((1ull << fp_len) -1) + 1;
    return h;
}

template <typename fp_t, int fp_len>
int MortonFilter<fp_t, fp_len>::get_count(int block_number, int index)
{
    return (T[block_number * 512 / 8 + 368 / 8 + (index * 2) / 8] >> (index * 2 % 8)) & 3;
}

template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::set_count(int block_number, int index, int count)
{
    T[block_number * 512 / 8 + 368 / 8 + (index * 2) / 8] = ((uint8_t) T[block_number * 512 / 8 + 368 / 8 + (index * 2) / 8] & (uint8_t)(~(3 << (index * 2 % 8)))) + (count << (index * 2 % 8));
}

template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::print_block(int pos)
{
    int base_byte = pos / 64 * (512 / 8);
    int block_number = pos / 64;

    printf("block number = %d\n", block_number);
    printf("FCA : ");
    for (int i = 0; i < 64; i++)
        printf("%d ", get_count(block_number, i));
    puts("");

    printf("FSA : ");
    for (int i = 0; i < 46; i++)
        printf("%x ", T[base_byte + i]);
    puts("");
}

template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::get_bucket(int pos, fp_t *store)
{
    // T : uint8_t *
    int block = pos / 64;
    int index = pos % 64;
    int base_byte = block * 512 / 8;
    int offset_index = 0;

    for (int i = 0; i < index; i++) offset_index += get_count(block, i);

    /*
    if (pos == 1099) 
        print_block(pos);
    */

    int cur_bucket_count = get_count(block, index);

    for (int i = 0; i < 5; i++) store[i] = 0;
    for (int i = 0; i < cur_bucket_count; i++)
        store[i] = T[base_byte + offset_index + i];

    for (int i = index; i < 64; i++) offset_index += get_count(block, i);

    store[3] = offset_index; // store block load
    for (int i = 0; i < 3; i++) store[4] += store[i] != 0; // bucket load
}


template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::set_bucket(int pos, fp_t *store)
{
    int block = pos / 64;
    int index = pos % 64;
    int base_byte = block * 512 / 8;
    int offset_index = 0;

    for (int i = 0; i < index; i++) offset_index += get_count(block, i);

    int cur_bucket_count = get_count(block, index);
    int new_bucket_count = 0;

    for (int i = 0; i < 3; i++)
        if (store[i] != 0)
            new_bucket_count++;

    if (new_bucket_count < cur_bucket_count) // delete elements, move forward
    { 
        printf("delete pos = %d, index = %d, old = %d, new = %d\n", pos, index, cur_bucket_count, new_bucket_count);
        //print_block(pos);
        for (int i = offset_index + new_bucket_count; i + (cur_bucket_count - new_bucket_count) < 368 / 8; i++)
        {
            if (i + cur_bucket_count - new_bucket_count >= 368 / 8)
                T[base_byte + i] = 0;
            else
                T[base_byte + i] = T[base_byte + i + cur_bucket_count - new_bucket_count];
        }
    } else 
    if (new_bucket_count > cur_bucket_count) // add elements, move backward
    {
        //printf("add element to pos = %d, index = %d, old = %d, new = %d\n", pos, index, cur_bucket_count, new_bucket_count);
        for (int i = 368 / 8 - 1; i >= offset_index + new_bucket_count; i--)
            T[base_byte + i] = T[base_byte + i - new_bucket_count + cur_bucket_count];
    }

    set_count(block, index, new_bucket_count);
   // printf("%d 0x%x\n", base_byte + 368 / 8 + (index * 2) / 8, T[base_byte + 368 / 8 + (index * 2) / 8]);

    //printf("store : ");

    for (int i = 0; i < new_bucket_count; i++)
    {
        //printf("0x%x ", store[i]);
        T[base_byte + offset_index + i] = store[i];
    }
    //puts("");

    //print_block(base_byte);
}

/*
template <typename fp_t, int fp_len>
fp_t CuckooFilter<fp_t, fp_len>::get_item(int pos, int rk)
{
    return (fp_t) this -> T[pos * this -> m + rk];
}

template <typename fp_t, int fp_len>
void CuckooFilter<fp_t, fp_len>::set_item(int pos, int rk, fp_t fp)
{
    this -> T[pos * this -> m + rk] = (fp_t) fp;
}
*/
template <typename fp_t, int fp_len>
int MortonFilter<fp_t, fp_len>::insert_to_bucket(fp_t *store, fp_t fp)
{

    // if success return 0
    // if fail return 1

    //get_bucket(pos, store);

    if (store[3] >= 368 / 8) return 1; // block full !

    for (int i = 0; i < this -> m; i++) 
        if (store[i] == 0)
        {
            store[i] = fp;
            //set_bucket(pos, store);
            return 0;
        }

    return 1;
}

template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::block_kick_one(int pos, int &alt, fp_t &fp)
{
    /*
    printf("ready to kick one\n");
    print_block(pos);
    */
    int block = pos / 64;
    int item_pos = (rand() % 46);
    int cnt = 0;
    int index = 0;
    for (int i = 0; i < 64; i++)
    {
        int t = get_count(block, i);
        if (cnt + t > item_pos)
        {
            index = i;
            break;
        }
        cnt += t;
    }

    int old_count = get_count(block, index);
    set_count(block, index, old_count - 1);

    fp = T[block * 512 / 8 + item_pos];
    for (int i = item_pos; i < 45; i++)
        T[block * 512 / 8 + i] = T[block * 512 / 8 + i + 1];

    T[block * 512 / 8 + 45] = 0;
    alt = index + block * 64;
}
template <typename fp_t, int fp_len>
int MortonFilter<fp_t, fp_len>::insert(uint64_t ele)
{

    // If insert success return 0
    // If insert fail return 1

    fp_t fp = fingerprint(ele);

    int cur1 = this -> position_hash(ele);
    int cur2 = alternate(cur1, fp);
    if (alternate(cur2, fp) != cur1)
    {
        deln(ele);
        deln(fp);
        printf("cur1 = %d alt cur1 = %d\n", cur1, alternate(cur1, fp));
        printf("cur2 = %d alt cur2 = %d\n", cur2, alternate(cur2, fp));
    }

    fp_t store1[8];
    fp_t store2[8];

    /*
    if (ele == 0x6229393a)
    {
        print_block(cur1);
        print_block(cur2);
        fp = fingerprint(0x6229393a);
    }
    */
    int cnt1 = 0, cnt2 = 0;
    get_bucket(cur1, store1);
    for (int i = 0; i < this -> m; i++) cnt1 += (store1[i] != 0);
    get_bucket(cur2, store2);
    for (int i = 0; i < this -> m; i++) cnt2 += (store2[i] != 0);

    if (insert_to_bucket(store1, fp) == 0) {filled_cell++; set_bucket(cur1, store1); return 0;}
    if (insert_to_bucket(store2, fp) == 0) {filled_cell++; set_bucket(cur2, store2); return 0;}

    //get those item
    int cur;
    fp_t *cur_store;

    if (rand() & 1)
        cur = cur1, cur_store = store1;
    else 
        cur = cur2, cur_store = store2;

    fp_t tmp_fp = cur_store[0];
    if (tmp_fp == 0)
    {
        // this block is full ! kick another item in this block
        int kick_pos = 0;
        fp_t kick_item = 0;
        /*
        double sb = this -> get_load_factor();
        deln(sb);
        */
        block_kick_one(cur, kick_pos, kick_item);

        cur_store[0] = fp;
        set_bucket(cur, cur_store);
        //printf("kick and store\n");
        //print_block(cur);

        cur = kick_pos;
        tmp_fp = kick_item;
    } else
    {
        cur_store[0] = fp;
        set_bucket(cur, cur_store);
    }

    int alt = alternate(cur, tmp_fp);
    int block_full = 0;
    int normal_kick = 0;
    vector<int> path;
    path.push_back(cur);
    
    for (int i = 0; i < this -> max_kick_steps; i++)
    {
        path.push_back(alt);
        memset(store1, 0, sizeof(store1));
        get_bucket(alt, store1);
        if (insert_to_bucket(store1, tmp_fp) == 0) 
        {
            filled_cell++; 
            set_bucket(alt, store1);
            return 0;
        }

        if (store1[4] < 3)
        {
            if (store1[3] != 46)
            {
                deln(store1[4]);
                deln(store1[3]);
            }
            assert(store1[3] == 46);
            block_full++;
            // this block is full ! kick another item in this block
            int kick_pos = 0;
            fp_t kick_item = 0;
            block_kick_one(alt, kick_pos, kick_item);

            get_bucket(alt, store1);
            bool ok = false;
            for (int i = 0; i < 3; i++)
                if (store1[i] == 0)
                {
                    store1[i] = tmp_fp;
                    ok = true;
                    break;
                }
            assert(ok);
            set_bucket(alt, store1);

            alt = kick_pos;
            fp = kick_item;
        } else
        {
            int t = rand() % 3;
            fp = store1[t];
            normal_kick++;
            store1[t] = tmp_fp;
            set_bucket(alt, store1);
        }


        tmp_fp = fp;
        alt = alternate(alt, tmp_fp);
    }

    /*
    for (int i = 0; i < this -> n; i += 64)
        print_block(i);

    for (int i = 0; i < path.size(); i++)
        printf("%d ", path[i]);
    */
    //puts("");
    deln(block_full);
    deln(normal_kick);

    return 1;
}

template <typename fp_t, int fp_len>
int MortonFilter<fp_t, fp_len>::lookup_in_bucket(int pos, fp_t fp)
{
    // If lookup success return 1
    // If lookup fail and the bucket is full return 2
    // If lookup fail and the bucket is not full return 3

    fp_t store[8];
    get_bucket(pos, store);

    int isFull = 1;
    for (int i = 0; i < this -> m; i++)
    {
        fp_t t = store[i];
        if (t == fp) 
            return 1;
        isFull &= (t != 0);
    }

    if (store[3] == 46) isFull = 1;
    return (isFull) ? 2 : 3;
}

template <typename fp_t, int fp_len>
bool MortonFilter<fp_t, fp_len>::lookup(uint64_t ele)
{

    // If ele is positive, return true
    // negative -- return false

	fp_t fp = fingerprint(ele);
    int pos1 = this -> position_hash(ele);
    int pos2 = alternate(pos1, fp);

    /*
    if (ele == 0x6229393a)
    {
        print_block(pos1);
        print_block(pos2);
        printf("fp = %x\n", fp);
    }
    */
  
    int ok1 = lookup_in_bucket(pos1, fp);

    if (ok1 == 1) return true;
    if (ok1 == 3) return false;

    int ok2 = lookup_in_bucket(pos2, fp);

    return ok2 == 1;
}

template <typename fp_t, int fp_len>
double MortonFilter<fp_t, fp_len>::get_load_factor()
{
    deln(filled_cell);
    deln(block_number);
    deln(block_number * 46);

    return filled_cell * 1.0 / (block_number * 368 / 8);
}

template <typename fp_t, int fp_len>
void MortonFilter<fp_t, fp_len>::test_bucket()
{
    for (int i = 0; i < this -> n; i++)
    {
        fp_t store[8];
        store[0] = rand() % (1 << 16);
        store[1] = rand() % (1 << 16);
        store[2] = rand() % (1 << 16);

        set_bucket(i, store);

        fp_t tmp_store[8];
        get_bucket(i, tmp_store);
        for (int j = 0; j < 3; j++)
            assert(tmp_store[j] == store[j]);
    }
}

template <typename fp_t, int fp_len>
class StandardCuckooFilter : public SemiSortCuckooFilter<fp_t, fp_len>
{
    private : 
    int alternate(int pos, fp_t fp) // get alternate position
    {
        int ret =  pos ^ this->position_hash(HashUtil::MurmurHash32(fp));
        //if (ret == pos)
        //    puts("cuckoo xor 0");
        return ret;
    }

    public : 
    int insert(uint64_t ele)
    {
        // If insert success return 0
        // If insert fail return 1
        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);

        fp_t fp = this -> fingerprint(ele);
        int cur1 = this -> position_hash(ele);
        int cur2 = this -> alternate(cur1, fp);

        fp_t store1[8];
        fp_t store2[8];

        this -> get_bucket(cur1, store1);
        this -> get_bucket(cur2, store2);

        if ((this -> insert_to_bucket(store1, fp)) == 0) {this -> filled_cell++; this -> set_bucket(cur1, store1); return 0;}
        if ((this -> insert_to_bucket(store2, fp)) == 0) {this -> filled_cell++; this -> set_bucket(cur2, store2); return 0;}

        // use bfs to search one non-full bucket

        static fp_t bucket[2000][4];
        static int bucket_pos[2000];
        int bucket_cnt = 0;

        static int index[3000];
        static int id[3000];
        static int prev[3000];
        static fp_t que_fp[3000];

        // index[i][0] : index in bucket array
        // id[i][1] : index in one bucket
        // prev[i][2] : previous status in bfs array
        // que_fp[i] : the fp queue!

        bool opt = false;

        int l = 0, r = 0, final_cnt = 100, p = -1;

        for (int i = 0; i < this -> m; i++) 
        {
            bucket[0][i] = store1[i];
            ++r;
            index[r] = 0;
            id[r] = i;
            prev[r] = -1;
            que_fp[r] = store1[i];
        }

        bucket_pos[0] = cur1;
        for (int i = 0; i < this -> m; i++) 
        {
            ++r;
            bucket[1][i] = store2[i];
            index[r] = 1;
            id[r] = i;
            prev[r] = -1;
            que_fp[r] = store2[i];
        }
        bucket_pos[1] = cur2;

        bucket_cnt = 1;
        bool quit = false;

        for (; l < r && r < this -> max_kick_steps * 4 && quit == false; )
        {
            ++l;
            fp_t cur_fp = que_fp[l];
            int pos = bucket_pos[index[l]];
            int alt = this -> alternate(pos, cur_fp);
            if (cur_fp == 0) continue;

            int bc = ++bucket_cnt;
            this -> get_bucket(alt, bucket[bc]);
            bucket_pos[bc] = alt;

            for (int i = 0; i < this -> m; i++) 
            {
                ++r;
                index[r] = bc;
                id[r] = i;
                prev[r] = l;
                que_fp[r] = bucket[bc][i];

                if (bucket[bc][i] == 0)
                {
                    // find a empty slot !
                    if (final_cnt > i)
                    {
                        final_cnt = i;
                        p = r;
                    }
                    // if opt : find the best
                    // if not opt : find the first
                    if (opt == false || final_cnt == 0) quit = true;
                    break;
                }
            }
        }
        if (final_cnt == 100) 
        {
            /*
            printf("fp = %x\n", fp);
            printf("que pos : \n");
            for (int i = 0; i < bucket_cnt; i++)
                printf("%d ", bucket_pos[i]);
            puts("");
            */
            return 1; // insert fail
        }

        for (; p > 0; p = prev[p])
        {
            if (prev[p] == -1)
                bucket[index[p]][id[p]] = fp; // move the original fp to the bucket
            else
                bucket[index[p]][id[p]] = que_fp[prev[p]]; // move the previous fp into current bucket !

            this -> set_bucket(bucket_pos[index[p]], bucket[index[p]]);
        }

        this -> filled_cell++;
        if (final_cnt == this -> m - 1)
            this -> full_bucket++;

        return 0; // succeed !
    }

}; 

template <typename fp_t, int fp_len>
class VacuumFilter : public SemiSortCuckooFilter<fp_t, fp_len>
{
    private : 
    virtual int alternate(int pos, fp_t fp) // get alternate position
    {
        //const static int len[4] = {long_seg - 1, 127, 31, 31};

        uint32_t fp_hash = fp * 0x5bd1e995;
        int seg = this -> len[fp & 3];
        /*
        switch (fp & 3)
        {
            case 0:
                seg = this -> big_seg;
                break;
            case 1:
                seg = 127;
                break;
            case 2: 
                seg = 31;
                break;
            case 3:
                seg = 31;
                break;
        }
        */
        return pos ^ (fp_hash & seg);
    }

    public : 

    int insert(uint64_t ele)
    {
        // If insert success return 0
        // If insert fail return 1

        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);

        fp_t fp = this -> fingerprint(ele);
        int cur1 = this -> position_hash(ele);
        int cur2 = alternate(cur1, fp);

        fp_t store1[8];
        fp_t store2[8];
        //fp_t store3[8];

        this -> get_bucket(cur1, store1);
        this -> get_bucket(cur2, store2);

        if (store1[this -> m] <= store2[this -> m])
        {
            if (this -> insert_to_bucket(store1, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur1, store1); return 0;}
        } else
        {
            if (this -> insert_to_bucket(store2, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur2, store2); return 0;}
        }

        //randomly choose one bucket's elements to kick
        int rk = rand() % this -> m;

        //get those item
        int cur;
        fp_t *cur_store;

        if (rand() & 1)
            cur = cur1, cur_store = store1;
        else 
            cur = cur2, cur_store = store2;

        fp_t tmp_fp = cur_store[rk];
        cur_store[rk] = fp;
        this -> set_bucket(cur, cur_store);

        int alt = alternate(cur, tmp_fp);
        
        for (int i = 0; i < this -> max_kick_steps; i++)
        {
            memset(store1, 0, sizeof(store1));
            this -> get_bucket(alt, store1);
            if (store1[this -> m] == this -> m)
            {
                for (int j = 0; j < this -> m; j++)
                {
                    int nex = alternate(alt, store1[j]);
                    this -> get_bucket(nex, store2);
                    if (store2[this -> m] < this -> m)
                    {
                        store2[this -> m - 1] = store1[j];
                        store1[j] = tmp_fp;
                        this -> filled_cell++;
                        this -> set_bucket(nex, store2);
                        this -> set_bucket(alt, store1);
                        return 0;
                    }
                }

                rk = rand() % this -> m;
                fp = store1[rk];
                store1[rk] = tmp_fp;
                this -> set_bucket(alt, store1);

                tmp_fp = fp;
                alt = alternate(alt, tmp_fp);
            } else
            {
                store1[this -> m - 1] = tmp_fp;
                this -> filled_cell++; 
                this -> set_bucket(alt, store1);
                return 0;
            }
        }
        return 1;
    }
/*
    int insert(uint64_t ele)
    {
        // If insert success return 0
        // If insert fail return 1
        //

        //if (this -> lookup(ele)) return 0;

        fp_t fp = this -> fingerprint(ele);
        int cur1 = this -> position_hash(ele);
        int cur2 = alternate(cur1, fp);

        if (alternate(cur2, fp) != cur1)
        {
            deln(ele);
            deln(fp);
            printf("cur1 = %d alt cur1 = %d\n", cur1, alternate(cur1, fp));
            printf("cur2 = %d alt cur2 = %d\n", cur2, alternate(cur2, fp));
        }

        fp_t store1[8];
        fp_t store2[8];

        this -> get_bucket(cur1, store1);
        this -> get_bucket(cur2, store2);

        if (store1[this -> m] <= store2[this -> m])
        {
            if (this -> insert_to_bucket(store1, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur1, store1); return 0;}
        } else
        {
            if (this -> insert_to_bucket(store2, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur2, store2); return 0;}
        }

        //randomly choose one bucket's elements to kick
        int rk = rand() % this -> m;

        //get those item
        int cur;
        fp_t *cur_store;
        fp_t buck[4][8];

        if (rand() & 1)
            cur = cur1, cur_store = store1;
        else 
            cur = cur2, cur_store = store2;

        fp_t tmp_fp = cur_store[rk];
        cur_store[rk] = fp;
        this -> set_bucket(cur, cur_store);

        int alt = alternate(cur, tmp_fp);
        this -> get_bucket(alt, store1);
        
        for (int i = 0; i < this -> max_kick_steps; i++)
        {
            //memset(store1, 0, sizeof(store1));
            //this -> get_bucket(alt, store1);
            if (store1[this -> m] == this -> m)
            {
                for (int j = 0; j < this -> m; j++)
                {
                    int nex = alternate(alt, store1[j]);
                    this -> get_bucket(nex, buck[j]);
                    if (buck[j][this -> m] < this -> m)
                    {
                        buck[j][this -> m - 1] = store1[j];
                        store1[j] = tmp_fp;
                        this -> filled_cell++;
                        this -> set_bucket(nex, buck[j]);
                        this -> set_bucket(alt, store1);
                        return 0;
                    }
                }

                rk = rand() % this -> m;
                fp = store1[rk];
                store1[rk] = tmp_fp;
                this -> set_bucket(alt, store1);

                tmp_fp = fp;
                alt = alternate(alt, tmp_fp);
                memcpy(store1, buck[rk], sizeof(store1));

            } else
            {
                store1[this -> m - 1] = tmp_fp;
                this -> filled_cell++; 
                this -> set_bucket(alt, store1);
                return 0;
            }
        }
        return 1;
    }

*/
/*
    int insert(uint64_t ele)
    {
        // If insert success return 0
        // If insert fail return 1

        if (this -> lookup(ele) == true)
        {
            ++(this -> filled_cell);
            return 0;
        }

        fp_t fp = this -> fingerprint(ele);
        int cur1 = this -> position_hash(ele);
        int cur2 = this -> alternate(cur1, fp);

        fp_t store1[8];
        fp_t store2[8];

        int cnt1 = 0, cnt2 = 0;
        int res1 = 0, res2 = 0;
        this -> get_bucket(cur1, store1);
        for (int i = 0; i < this -> m; i++) cnt1 += (store1[i] != 0);
        this -> get_bucket(cur2, store2);
        for (int i = 0; i < this -> m; i++) cnt2 += (store2[i] != 0);

        if (cnt1 <= cnt2)
        {
            if ((res1 = this -> insert_to_bucket(store1, fp)) == 0) {this -> filled_cell++; this -> set_bucket(cur1, store1); return 0;}
            //if ((res2 = insert_to_bucket(store2, fp)) == 0) {filled_cell++; set_bucket(cur2, store2); return 0;}
        } else
        {
            if ((res2 = this -> insert_to_bucket(store2, fp)) == 0) {this -> filled_cell++; this -> set_bucket(cur2, store2); return 0;}
            //if ((res1 = insert_to_bucket(store1, fp)) == 0) {filled_cell++; set_bucket(cur1, store1); return 0;}
        }

        // use bfs to search one non-full bucket

        static fp_t bucket[2000][4];
        static int bucket_pos[2000];
        int bucket_cnt = 0;

        static int index[3000];
        static int id[3000];
        static int prev[3000];
        static fp_t que_fp[3000];

        // index[i][0] : index in bucket array
        // id[i][1] : index in one bucket
        // prev[i][2] : previous status in bfs array
        // que_fp[i] : the fp queue!

        bool opt = (this -> get_load_factor() > 0.95) ? true : false;

        int l = 0, r = 0, final_cnt = 100, p = -1;

        for (int i = 0; i < this -> m; i++) 
        {
            bucket[0][i] = store1[i];
            ++r;
            index[r] = 0;
            id[r] = i;
            prev[r] = -1;
            que_fp[r] = store1[i];
        }

        bucket_pos[0] = cur1;
        for (int i = 0; i < this -> m; i++) 
        {
            ++r;
            bucket[1][i] = store2[i];
            index[r] = 1;
            id[r] = i;
            prev[r] = -1;
            que_fp[r] = store2[i];
        }
        bucket_pos[1] = cur2;

        bucket_cnt = 1;
        bool quit = false;

        for (; l < r && r < this -> max_kick_steps * 4 && quit == false; )
        {
            ++l;
            fp_t cur_fp = que_fp[l];
            int pos = bucket_pos[index[l]];
            int alt = this -> alternate(pos, cur_fp);
            if (cur_fp == 0) continue;

            int bc = ++bucket_cnt;
            this -> get_bucket(alt, bucket[bc]);
            bucket_pos[bc] = alt;

            for (int i = 0; i < this -> m; i++) 
            {
                ++r;
                index[r] = bc;
                id[r] = i;
                prev[r] = l;
                que_fp[r] = bucket[bc][i];

                if (bucket[bc][i] == 0)
                {
                    // find a empty slot !
                    if (final_cnt > i)
                    {
                        final_cnt = i;
                        p = r;
                    }
                    // if opt : find the best
                    // if not opt : find the first
                    if (opt == false || final_cnt == 0) quit = true;
                    break;
                }
            }
        }
        if (final_cnt == 100) 
        {
            printf("fp = %x\n", fp);
            printf("que pos : \n");
            for (int i = 0; i < bucket_cnt; i++)
                printf("%d ", bucket_pos[i]);
            puts("");
            return 1; // insert fail
        }

        for (; p > 0; p = prev[p])
        {
            if (prev[p] == -1)
                bucket[index[p]][id[p]] = fp; // move the original fp to the bucket
            else
                bucket[index[p]][id[p]] = que_fp[prev[p]]; // move the previous fp into current bucket !

            this -> set_bucket(bucket_pos[index[p]], bucket[index[p]]);
        }

        this -> filled_cell++;
        if (final_cnt == this -> m - 1)
            this -> full_bucket++;

        return 0; // succeed !
    }
*/
}; 

template <typename fp_t, int fp_len>
class MortonAddFilter : public MortonFilter<fp_t, fp_len>
{
    private : 

    int alternate(int pos, fp_t fp)
    {
        // My cuckoo filter -- plus or minus
		//int n = this->n;
        //int B;
        //int bias = (n / (1 << fp_len)) * fp;  // bias -- avoid aggregate
		//pos = (pos + bias) % n;
        int delta = ((HashUtil::MurmurHash32(fp) & (this -> max_2_power - 1)) + 64) | 1;
        int ret = 0;

        if (pos & 1)
            ret = pos - delta;
        else ret = pos + delta;

        return this -> position_hash(ret);

    }
};
template <typename fp_t, int fp_len>
class OffsetFilter : public SemiSortCuckooFilter<fp_t, fp_len>
{
    private : 
    int alternate(int pos, fp_t fp)
    {
        // My cuckoo filter -- plus or minus
		//int n = this->n;
        //int B;
        //int bias = (n / (1 << fp_len)) * fp;  // bias -- avoid aggregate
		//pos = (pos + bias) % n;
        //int delta = ((HashUtil::MurmurHash32(fp) & (this -> max_2_power - 1)) + 64) | 1;
        int delta = ((HashUtil::MurmurHash32(fp) & (this -> max_2_power - 1)));

        pos = pos - delta;
        if (pos < 0)  pos += this -> n;

        pos = this -> n - 1 - pos;

        pos = pos + delta;
        if (pos >= this -> n) pos -= this -> n;

        return pos;

        /*
        if (pos & 1)
            ret = pos - delta;
        else ret = pos + delta;

        return this -> position_hash(ret);
        */

    }
    public : 


    int insert(uint64_t ele)
    {
        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);
        if (this -> lookup(ele) == 1)
        {
            //++this -> filled_cell;
            return 0;
        }
        // If insert success return 0
        // If insert fail return 1

        fp_t fp = this -> fingerprint(ele);
        int cur1 = this -> position_hash(ele);
        int cur2 = alternate(cur1, fp);

        fp_t store1[8];
        fp_t store2[8];
        //fp_t store3[8];

        this -> get_bucket(cur1, store1);
        this -> get_bucket(cur2, store2);

        if (store1[this -> m] <= store2[this -> m])
        {
            if (this -> insert_to_bucket(store1, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur1, store1); return 0;}
        } else
        {
            if (this -> insert_to_bucket(store2, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur2, store2); return 0;}
        }

        // look foward once
        /*
        for (int j = 0; j < this -> m; j++)
        {
            int nex = alternate(cur1, store1[j]);
            this -> get_bucket(nex, store3);
            if (store3[this -> m] < this -> m)
            {
                store3[this -> m - 1] = store1[j];
                store1[j] = fp;
                this -> filled_cell++;
                this -> set_bucket(nex, store3);
                this -> set_bucket(cur1, store1);
                return 0;
            }
        }

        for (int j = 0; j < this -> m; j++)
        {
            int nex = alternate(cur2, store2[j]);
            this -> get_bucket(nex, store3);
            if (store3[this -> m] < this -> m)
            {
                store3[this -> m - 1] = store2[j];
                store2[j] = fp;
                this -> filled_cell++;
                this -> set_bucket(nex, store3);
                this -> set_bucket(cur2, store2);
                return 0;
            }
        }
        */

        //randomly choose one bucket's elements to kick
        int rk = rand() % this -> m;

        //get those item
        int cur;
        fp_t *cur_store;

        if (rand() & 1)
            cur = cur1, cur_store = store1;
        else 
            cur = cur2, cur_store = store2;

        fp_t tmp_fp = cur_store[rk];
        cur_store[rk] = fp;
        this -> set_bucket(cur, cur_store);

        int alt = alternate(cur, tmp_fp);
        
        for (int i = 0; i < this -> max_kick_steps; i++)
        {
            memset(store1, 0, sizeof(store1));
            this -> get_bucket(alt, store1);
            if (store1[this -> m] == this -> m)
            {
                for (int j = 0; j < this -> m; j++)
                {
                    int nex = alternate(alt, store1[j]);
                    this -> get_bucket(nex, store2);
                    if (store2[this -> m] < this -> m)
                    {
                        store2[this -> m - 1] = store1[j];
                        store1[j] = tmp_fp;
                        this -> filled_cell++;
                        this -> set_bucket(nex, store2);
                        this -> set_bucket(alt, store1);
                        return 0;
                    }
                }

                rk = rand() % this -> m;
                fp = store1[rk];
                store1[rk] = tmp_fp;
                this -> set_bucket(alt, store1);

                tmp_fp = fp;
                alt = alternate(alt, tmp_fp);
            } else
            {
                store1[this -> m - 1] = tmp_fp;
                this -> filled_cell++; 
                this -> set_bucket(alt, store1);
                return 0;
            }
        }
        return 1;
    }
};

template <typename fp_t, int fp_len>
class RandomFilter : public SemiSortCuckooFilter<fp_t, fp_len>
{
    public : 
    
    int alt[1100][256];
    int id[1100];

    void init(int _n, int _m, int _steps)
    {
        SemiSortCuckooFilter<fp_t, fp_len>::init(_n, _m, _steps);
        for (int i = 0; i < _n; i++) id[i] = i;
        for (int i = 0; i < (1 << fp_len); i++)
        {
            random_shuffle(id, id + _n);
            for (int j = 0; j < _n; j += 2)
            {
                //printf("%d %d ", id[j], id[j + 1]);
                alt[id[j]][i] = id[j + 1], alt[id[j + 1]][i] = id[j];
            }
            //puts("");
        }
    }

    private : 

    int alternate(int pos, fp_t fp)
    {
        return alt[pos][fp];
    }
};

template <typename fp_t, int fp_len>
class AddSubFilter : public SemiSortCuckooFilter<fp_t, fp_len>
{
    private : 

    int alternate(int pos, fp_t fp)
    {
        // My cuckoo filter -- plus or minus
		int n = this->n;
        int bias = (n / (1 << fp_len)) * fp;  // bias -- avoid aggregate
		pos = (pos + bias) % n;
        int delta = HashUtil::MurmurHash32(fp) % (n-1) + 1;

        int sum_bound = 0;
        int ret = 0;

        int nn = n;

        for (; ;)
        {
            if (((pos / delta) & 1) == 0) 
            {
                if (pos + delta < nn)
                {
                    ret = pos + delta;
                    break;
                } else
                {
                    int lb = max((pos / delta) * delta, nn - delta);
                    int ub = min((pos / delta) * delta + delta - 1, nn - 1); // [lb, ub] is the interval which can't determine pair
                    sum_bound += lb;
                    nn = ub - lb + 1;
                    delta = HashUtil::MurmurHash32(delta) % (nn-1) + 1;
                    pos = pos - lb;
                }
            }
            else
            {
                ret = pos - delta; // case : minus delta -- must be legal !
                break;
            }
        }

        ret = ret + sum_bound;
        ret = (ret - bias + n) % n;
        return ret;
    }
};

template <typename fp_t, int fp_len>
class VacuumFilterTest : public VacuumFilter<fp_t, fp_len>
{
    private : 

    int len[4];

    int alternate(int pos, fp_t fp) // get alternate position
    {

        //int n = this -> n;
        int fp_hash = fp * 0x5bd1e995;

        //int bias = fp_hash & (this -> max_2_power - 1);

        //int segment_length = 64;
        //if ((fp & 3) == 0) segment_length = 2048;
        int segment_length = len[fp & 3];

        int t = (fp_hash & (segment_length - 1));
        if (t == 0) t = 1;

        /*
        pos = pos + bias;
        if (pos >= n) pos -= n;
        */
        
        //int segment_length = 1 << find_highest_bit(pos ^ n);
        //int segment_length = lower_power_of_two((pos ^ n));
        int ret = pos ^ t;

        /*
        ret = ret - bias;
        if (ret < 0) ret += n;
        */
        
        return ret;
    }

    public :
    void set_alt_len(int x)
    {
        //len[0] = len[1] = len[2] = len[3] = x;
        len[0] = x;
        len[1] = 128;
        len[2] = len[3] = 32;
    }
}; 


#endif
