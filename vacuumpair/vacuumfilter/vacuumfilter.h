#ifndef CUCKOO_FILTER_CUCKOO_FILTER_H_
#define CUCKOO_FILTER_CUCKOO_FILTER_H_

#include <assert.h>
#include <algorithm>

#include "debug.h"
#include "hashutil.h"
#include "packedtable.h"
#include "printutil.h"
#include "singletable.h"

#define ROUNDDOWN(a, b) ((a) - ((a) % (b)))
#define ROUNDUP(a, b) ROUNDDOWN((a) + (b - 1), b)
//const int big_seg = 262144;
const int AR = 4;
const int BATCH_SIZE = 128;

namespace cuckoofilter
{
  // status returned by a cuckoo filter operation

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
    double b = 4;     // slots per bucket
    double lf = 0.95; // target load factor
    int alt_range = 8;
    for (; alt_range < M;)
    {
      double f = (AR - i) * (1.0 / AR);
      /*
      for (int j = 0; j < i; j++)
          f += 0.25 * (alt_range * 1.0 / len[j]);
      */
      if (balls_in_bins_max_load(f * b * lf * M, M * 1.0 / alt_range) < 0.97 * b * alt_range) // Alg. 1: if D < P
        return alt_range;
      alt_range <<= 1;
    }
    return -1;
  }
  enum Status
  {
    Ok = 0,
    NotFound = 1,
    NotEnoughSpace = 2,
    NotSupported = 3,
  };

  // maximum number of cuckoo kicks before claiming failure
  const size_t kMaxCuckooCount = 500;

  // A cuckoo filter class exposes a Bloomier filter interface,
  // providing methods of Add, Delete, Contain. It takes three
  // template parameters:
  //   ItemType:  the type of item you want to insert
  //   bits_per_item: how many bits each item is hashed into
  //   TableType: the storage of table, SingleTable by default, and
  // PackedTable to enable semi-sorting
  template <typename ItemType, size_t bits_per_item,
            typename HashFamily = TwoIndependentMultiplyShift,
            template <size_t> class TableType = SingleTable>
  class VacuumFilter
  {
    // Storage of items
    TableType<bits_per_item> *table_;

    // Number of items stored
    size_t num_items_;
    size_t max_2_power;
    size_t alt_len;
    bool packed;

    typedef struct
    {
      size_t index;
      uint32_t tag;
      bool used;
    } VictimCache;

    VictimCache victim_;

    HashFamily hasher_;

    std::vector<uint8_t> seeds_;

    int big_seg;
    int len[AR];

    inline size_t IndexHash(const ItemType &item) const
    {
      const uint32_t hash = item >> 32;
      return (((uint64_t)hash * (uint64_t)table_->NumBuckets()) >> 32);
    }

    inline uint32_t TagHash(uint32_t hv) const
    {
      uint32_t tag;
      tag = hv & ((1ULL << bits_per_item) - 1);
      tag += (tag == 0);
      return tag;
    }

    inline void GenerateIndexTagHash(const ItemType &item, size_t *index,
                                     uint32_t *tag) const
    {
      *index = IndexHash(item);
      const uint64_t hash = hasher_(item, seeds_.at(*index));
      *tag = TagHash(hash);
    }

    // modified of above function to use for 2 indices and tags
    inline void GenerateAltIndexTagHash(const ItemType &item, const size_t i1, size_t *i2,
                                  uint32_t *tag2) const
    {
      // *i1 = IndexHash(item); // original: hash >> 32
      *i2 = AltIndex(i1, item);
      const uint64_t hash2 = hasher_(item, seeds_.at(*i2));
      *tag2 = TagHash(hash2);
    }

    inline size_t AltIndex(int index, const ItemType &item) const
    {
      // NOTE(binfan): originally we use:
      // index ^ HashUtil::BobHash((const void*) (&tag), 4)) & table_->INDEXMASK;
      // now doing a quick-n-dirty way:
      // 0x5bd1e995 is the hash constant from MurmurHash2
      //return IndexHash((uint32_t)(index ^ (tag * 0x5bd1e995)));
      int t = item * 0x5bd1e995;
      int seg = len[item & (AR - 1)];
      t = t & seg;
      //t += (t == 0);
      return index ^ t;
    }

    Status AddImpl(const size_t i, const uint32_t tag);

  public:
    explicit VacuumFilter(const size_t max_num_keys, bool aligned = false, bool _packed = false) : num_items_(0), victim_(), hasher_()
    {

      std::cout << "good" << std::endl;
      srand(1);
      size_t assoc = 4;
      size_t num_buckets;
      packed = _packed;

      if (aligned)
      {
        num_buckets = upperpower2(std::max<uint64_t>(1, max_num_keys / assoc));
        if (num_buckets < 128)
          num_buckets = 128;
        //size_t num_buckets = std::max<uint64_t>(1, max_num_keys / assoc);
        //if (num_buckets & 1) num_buckets += 1;
        double frac = (double)max_num_keys / num_buckets / assoc;

        if (frac > 0.96)
        {
          //num_buckets <<= 1;
        }
        for (int i = 0; i < AR; i++)
          len[i] = num_buckets - 1;
      }
      else
      { // this is default
        /*
            big_seg = upperpower2(int(max_num_keys / assoc / 0.95) / 100);
            if (big_seg < 128) big_seg = 128;
            num_buckets = ROUNDUP(int(max_num_keys / assoc / 0.95), big_seg);
        */
        //big_seg = upperpower2(int(max_num_keys / assoc) / 100);
        big_seg = 0;
        big_seg = std::max(big_seg, proper_alt_range(max_num_keys / assoc, 0, len));
        big_seg = std::max(big_seg, 1024);
        num_buckets = ROUNDUP(int(max_num_keys / assoc), big_seg);

        big_seg--;
        len[0] = big_seg;
        for (int i = 1; i < AR; i++) // sets all alt ranges
          len[i] = proper_alt_range(num_buckets, i, len) - 1;
        len[AR - 1] = (len[AR - 1] + 1) * 2 - 1;
        if (AR == 1)
          num_buckets = ROUNDUP(int(max_num_keys / assoc), len[0] + 1);
      }

      /* 
    len[0] = big_seg;
    len[1] = proper_alt_range(num_buckets, 0.75) - 1;
    len[2] = proper_alt_range(num_buckets, 0.5) - 1;
    len[3] = proper_alt_range(num_buckets, 0.32) - 1;
    */

      victim_.used = false;

      /*
    max_2_power = 1;
    for (; max_2_power * 2 <= num_buckets; ) max_2_power <<= 1;
    alt_len = max_2_power >> 7;
    if (alt_len < 64) alt_len = 64;
    alt_len--;
    */

      std::cout << "num_buckets = " << num_buckets << std::endl;
      //std::cout << "big_segment_length = " << big_seg << std::endl;
      std::cout << "alt range setting : ";
      for (int i = 0; i < AR; i++)
        std::cout << len[i] + 1 << ", ";
      std::cout << std::endl;

      table_ = new TableType<bits_per_item>(num_buckets);
    }
    
    // modified constructor
    explicit VacuumFilter(const size_t max_num_keys, const std::vector<uint8_t> &seeds = std::vector<uint8_t>(), bool aligned = false, bool _packed = false) : num_items_(0), seeds_(seeds), victim_(), hasher_()
    {

      // std::cout << "good" << std::endl;
      srand(1);
      size_t assoc = 4;
      size_t num_buckets = seeds_.size();
      // size_t num_buckets;
      packed = _packed;

      big_seg = 0;
      big_seg = std::max(big_seg, proper_alt_range(max_num_keys / assoc, 0, len));
      big_seg = std::max(big_seg, 1024);
      // num_buckets = ROUNDUP(int(max_num_keys / assoc), big_seg);

      big_seg--;
      len[0] = big_seg;
      for (int i = 1; i < AR; i++) // sets all alt ranges
        len[i] = proper_alt_range(num_buckets, i, len) - 1;
      len[AR - 1] = (len[AR - 1] + 1) * 2 - 1;
      // if (AR == 1)
      //   num_buckets = ROUNDUP(int(max_num_keys / assoc), len[0] + 1);

      // assert(seeds_.size() == num_buckets); // double-check calculations match
      victim_.used = false;

      std::cout << "num_buckets = " << num_buckets << std::endl;
      //std::cout << "big_segment_length = " << big_seg << std::endl;
      std::cout << "alt range setting : ";
      for (int i = 0; i < AR; i++)
        std::cout << len[i] + 1 << ", ";
      std::cout << std::endl;

      table_ = new TableType<bits_per_item>(num_buckets);
    }

    ~VacuumFilter() { delete table_; }

    // Add an item to the filter.
    Status Add(const ItemType &item);

    // Add an item to the filter.
    void Add_many(const ItemType *item, bool *result, int key_n);

    bool evict(const size_t i, const uint32_t tag);

    // Insert item to the filter at given bucket index and slot.
    Status CopyInsert(const uint32_t fp, size_t index, size_t slot);

    // Report if the item is inserted, with false positive rate.
    Status Contain(const ItemType &item) const;

    // Report if the item is inserted, with false positive rate.
    void Contain_many(const ItemType *item, bool *result, int key_n);

    // Report if the item is inserted, report bucket 1 or bucket 2
    uint32_t Contain_where(const ItemType &item) const;

    // Delete an key from the filter
    Status Delete(const ItemType &item);

    // Delete an key from the filter
    void Delete_many(const ItemType *item, bool *result, int key_n);

    size_t SeedTable_Size() const;

    /* methods for providing stats  */
    // summary infomation
    std::string Info() const;

    // number of current inserted items;
    size_t Size() const { return num_items_; }

    // size of the filter in bytes.
    size_t SizeInBytes() const { return table_->SizeInBytes(); }

    // load factor is the fraction of occupancy
    double LoadFactor() const { return 1.0 * Size() / table_->SizeInTags(); }

    double BitsPerItem() const { return 8.0 * (table_->SizeInBytes() + SeedTable_Size()) / Size(); }
  };

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  Status VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Add(
      const ItemType &item)
  {
    size_t i;
    uint32_t tag;

    if (victim_.used)
    {
      return NotEnoughSpace;
    }

    GenerateIndexTagHash(item, &i, &tag);
    return AddImpl(i, tag);
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  void VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Add_many(
      const ItemType *key, bool *result, int key_n)
  {

    size_t bucket_position[BATCH_SIZE];
    size_t alt_position[BATCH_SIZE];
    uint32_t tag[BATCH_SIZE];
    bool tmp_result[BATCH_SIZE];
    uint32_t oldtag;
    uint32_t tags[4];

    for (int i = 0; i < key_n; i += BATCH_SIZE)
    {
      int up = std::min(BATCH_SIZE, key_n - i);
      memset(tmp_result, false, BATCH_SIZE);
      for (int j = 0; j < up; j++)
        GenerateIndexTagHash(key[i + j], &bucket_position[j], &tag[j]);
      for (int j = 0; j < up; j++)
        tmp_result[j] = table_->InsertTagToBucket(bucket_position[j], tag[j], false, oldtag, tags);
      for (int j = 0; j < up; j++)
        alt_position[j] = AltIndex(bucket_position[j], tag[j]);
      for (int j = 0; j < up; j++)
        if (tmp_result[j] == false)
          tmp_result[j] = table_->InsertTagToBucket(alt_position[j], tag[j], false, oldtag, tags);
      for (int j = 0; j < up; j++)
        if (tmp_result[j] == false)
          tmp_result[j] = evict(bucket_position[j], tag[j]);
      for (int j = 0; j < up; j++)
        num_items_ += tmp_result[j];
      memcpy(&result[i], tmp_result, BATCH_SIZE);
    }
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  bool VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::evict(
      const size_t i, const uint32_t tag)
  {
    size_t curindex = i;
    uint32_t curtag = tag;
    uint32_t oldtag;

    uint32_t tags[4];
    uint32_t tmp_tags[4];

    //table_->ReadBucket(curindex, tags);

    for (uint32_t count = 0; count < kMaxCuckooCount; count++)
    {
      //bool kickout = count > 0;
      oldtag = 0;
      if (table_->InsertTagToBucket(curindex, curtag, false, oldtag, tags))
      {
        //num_items_++;
        return true;
      }
      for (int i = 0; i < 4; i++)
      {
        int alt = AltIndex(curindex, tags[i]);
        if (table_->InsertTagToBucket(alt, tags[i], false, oldtag, tmp_tags))
        {
          tags[i] = curtag;
          table_->WriteBucket(curindex, tags, true, i);

          //num_items_++;
          return true;
        }
      }

      int r = rand() % 4;
      oldtag = tags[r];
      tags[r] = curtag;
      table_->WriteBucket(curindex, tags, true, r);
      curtag = oldtag;
      curindex = AltIndex(curindex, curtag);

      /*
    tags[0] = tmp_tags[r][0];
    tags[1] = tmp_tags[r][1];
    tags[2] = tmp_tags[r][2];
    tags[3] = tmp_tags[r][3];
*/
    }

    return false;
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  Status VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::AddImpl(
      const size_t i, const uint32_t tag)
  {
    size_t curindex = i;
    uint32_t curtag = tag;
    uint32_t oldtag;

    uint32_t tags[4];
    uint32_t tmp_tags[4];

    // this insert seems redundant, handled in first iter. of loop below?
    if (table_->InsertTagToBucket(curindex, curtag, false, oldtag, tags))
    {
      num_items_++;
      return Ok;
    }

    uint32_t altbucket = AltIndex(curindex, curtag);
    if (table_->InsertTagToBucket(altbucket, curtag, false, oldtag, tags))
    {
      num_items_++;
      return Ok;
    }
    //curindex = AltIndex(curindex, curtag);

    for (uint32_t count = 0; count < kMaxCuckooCount; count++)
    {
      //bool kickout = count > 0;
      oldtag = 0;
      if (table_->InsertTagToBucket(curindex, curtag, false, oldtag, tags))
      {
        num_items_++;
        return Ok;
      }
      for (int i = 0; i < 4; i++)
      {
        int alt = AltIndex(curindex, tags[i]);
        if (table_->InsertTagToBucket(alt, tags[i], false, oldtag, tmp_tags))
        {
          tags[i] = curtag;
          table_->WriteBucket(curindex, tags, true, i);

          num_items_++;
          return Ok;
        }
      }

      int r = rand() % 4;
      oldtag = tags[r];
      tags[r] = curtag;
      table_->WriteBucket(curindex, tags, true, r);
      curtag = oldtag;
      curindex = AltIndex(curindex, curtag);
    }

    victim_.index = curindex;
    victim_.tag = curtag;
    victim_.used = true;
    return Ok;
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
Status VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::CopyInsert(
    const uint32_t fp, const size_t index, const size_t slot) {
  if (table_->CopyTagToBucket(index, slot, fp)) {
    // table_->WriteTag(index, slot, fp);
    num_items_++;
    // std::cout << "copied " << fp << " to " << index << "\n";
    return Ok;
  }
  return NotSupported;
}

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  Status VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Contain(
      const ItemType &key) const
  {
    // bool found = false;
    size_t i1, i2;
    uint32_t tag1, tag2;

    GenerateIndexTagHash(key, &i1, &tag1);

    if (table_->FindTagInBucket(i1, tag1))
      return Ok;

    // size_t i2;
    // uint32_t tag2;
    GenerateAltIndexTagHash(key, i1, &i2, &tag2);
    if (table_->FindTagInBucket(i2, tag2))
      return Ok;

    return NotFound;
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  void VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Contain_many(
      const ItemType *key, bool *result, int key_n)
  {

    size_t bucket_position[BATCH_SIZE];
    uint32_t tag[BATCH_SIZE];
    bool tmp_result[BATCH_SIZE];

    for (int i = 0; i < key_n; i += BATCH_SIZE)
    {
      memset(tmp_result, false, BATCH_SIZE);
      for (int j = 0; j < BATCH_SIZE; j++)
        GenerateIndexTagHash(key[i + j], &bucket_position[j], &tag[j]);
      for (int j = 0; j < BATCH_SIZE; j++)
        tmp_result[j] = table_->FindTagInBucket(bucket_position[j], tag[j]);
      for (int j = 0; j < BATCH_SIZE; j++)
        bucket_position[j] = AltIndex(bucket_position[j], tag[j]);
      for (int j = 0; j < BATCH_SIZE; j++)
        if (tmp_result[j] == false)
          tmp_result[j] = table_->FindTagInBucket(bucket_position[j], tag[j]);

      memcpy(&result[i], tmp_result, BATCH_SIZE);
    }
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  uint32_t VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Contain_where(
      const ItemType &key) const
  {
    bool found = false;
    size_t i1, i2;
    uint32_t tag;

    GenerateIndexTagHash(key, &i1, &tag);

    if ((victim_.used && i1 == victim_.index && tag == victim_.tag) || table_->FindTagInBucket(i1, tag))
      return 1;

    i2 = AltIndex(i1, tag);

    if ((victim_.used && i2 == victim_.index && tag == victim_.tag) || table_->FindTagInBucket(i2, tag))
      return 2;

    return 0;
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  void VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Delete_many(
      const ItemType *key, bool *result, int key_n)
  {

    size_t bucket_position[BATCH_SIZE];
    uint32_t tag[BATCH_SIZE];
    bool tmp_result[BATCH_SIZE];

    for (int i = 0; i < key_n; i += BATCH_SIZE)
    {
      int up = std::min(BATCH_SIZE, key_n - i);
      memset(tmp_result, false, BATCH_SIZE);
      for (int j = 0; j < up; j++)
        GenerateIndexTagHash(key[i + j], &bucket_position[j], &tag[j]);
      for (int j = 0; j < up; j++)
        tmp_result[j] = table_->DeleteTagFromBucket(bucket_position[j], tag[j]);
      for (int j = 0; j < up; j++)
        bucket_position[j] = AltIndex(bucket_position[j], tag[j]);
      for (int j = 0; j < up; j++)
        if (tmp_result[j] == false)
          tmp_result[j] = table_->DeleteTagFromBucket(bucket_position[j], tag[j]);
      for (int j = 0; j < up; j++)
        num_items_ -= tmp_result[j];
      memcpy(&result[i], tmp_result, BATCH_SIZE);
    }
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  Status VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Delete(
      const ItemType &key)
  {
    size_t i1, i2;
    uint32_t tag;

    GenerateIndexTagHash(key, &i1, &tag);

    if (table_->DeleteTagFromBucket(i1, tag))
    {
      num_items_--;
      goto TryEliminateVictim;
    }
    i2 = AltIndex(i1, tag);
    if (table_->DeleteTagFromBucket(i2, tag))
    {
      num_items_--;
      goto TryEliminateVictim;
    }
    else if (victim_.used && tag == victim_.tag &&
             (i1 == victim_.index || i2 == victim_.index))
    {
      // num_items_--;
      victim_.used = false;
      return Ok;
    }
    else
    {
      return NotFound;
    }
  TryEliminateVictim:
    if (victim_.used)
    {
      victim_.used = false;
      size_t i = victim_.index;
      uint32_t tag = victim_.tag;
      AddImpl(i, tag);
    }
    return Ok;
  }

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
size_t VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::SeedTable_Size() const {
  return sizeof(std::vector<uint8_t>) + (sizeof(uint8_t) * seeds_.size());
  // sizeof(vector) = 24, sizeof(uint8_t) = 1, sizeof(int) = 4
}

  template <typename ItemType, size_t bits_per_item, typename HashFamily,
            template <size_t> class TableType>
  std::string VacuumFilter<ItemType, bits_per_item, HashFamily, TableType>::Info() const
  {
    std::stringstream ss;
    size_t table_size = table_->SizeInBytes();
    size_t seedtable_size = SeedTable_Size();
    size_t total_size = table_size + seedtable_size;
    ss << "VacuumFilter Status:\n"
       << "\t\t" << table_->Info() << "\n"
       << "\t\tKeys stored: " << Size() << "\n"
       << "\t\tLoad factor: " << LoadFactor() << "\n"
      //  << "\t\tHashtable size: " << (table_->SizeInBytes() >> 10) << " KB\n";
      << "\t\tHashtable size: " 
     << table_size << " (table) + " << seedtable_size << " (seeds) = " << total_size << " bytes ("
     << (table_size >> 10) << " KB)\n";
    if (Size() > 0)
    {
      ss << "\t\tbit/key:   " << BitsPerItem() << "\n";
    }
    else
    {
      ss << "\t\tbit/key:   N/A\n";
    }
    return ss.str();
  }
} // namespace cuckoofilter

namespace cuckoofilter
{
  // status returned by a cuckoo filter operation
  /*
enum Status {
  Ok = 0,
  NotFound = 1,
  NotEnoughSpace = 2,
  NotSupported = 3,
};
*/

  // maximum number of cuckoo kicks before claiming failure
  //const size_t kMaxCuckooCount = 500;

  // A cuckoo filter class exposes a Bloomier filter interface,
  // providing methods of Add, Delete, Contain. It takes three
  // template parameters:
  //   ItemType:  the type of item you want to insert
  //   bits_per_item: how many bits each item is hashed into
  //   TableType: the storage of table, SingleTable by default, and
  // PackedTable to enable semi-sorting
  template <typename ItemType, size_t bits_per_item,
            template <size_t> class TableType = SingleTable,
            typename HashFamily = TwoIndependentMultiplyShift>
  class CuckooFilter
  {
    // Storage of items
    TableType<bits_per_item> *table_;

    // Number of items stored
    size_t num_items_;

    typedef struct
    {
      size_t index;
      uint32_t tag;
      bool used;
    } VictimCache;

    VictimCache victim_;

    HashFamily hasher_;

    inline size_t IndexHash(uint32_t hv) const
    {
      // table_->num_buckets is always a power of two, so modulo can be replaced
      // with
      // bitwise-and:
      return hv & (table_->NumBuckets() - 1);
    }

    inline uint32_t TagHash(uint32_t hv) const
    {
      uint32_t tag;
      tag = hv & ((1ULL << bits_per_item) - 1);
      tag += (tag == 0);
      return tag;
    }

    inline void GenerateIndexTagHash(const ItemType &item, size_t *index,
                                     uint32_t *tag) const
    {
      const uint64_t hash = hasher_(item);
      *index = IndexHash(hash >> 32);
      *tag = TagHash(hash);
    }

    inline size_t AltIndex(const size_t index, const uint32_t tag) const
    {
      // NOTE(binfan): originally we use:
      // index ^ HashUtil::BobHash((const void*) (&tag), 4)) & table_->INDEXMASK;
      // now doing a quick-n-dirty way:
      // 0x5bd1e995 is the hash constant from MurmurHash2
      return IndexHash((uint32_t)(index ^ (tag * 0x5bd1e995)));
    }

    Status AddImpl(const size_t i, const uint32_t tag);

    // load factor is the fraction of occupancy
  public:
    double LoadFactor() const { return 1.0 * Size() / table_->SizeInTags(); }

    double BitsPerItem() const { return 8.0 * table_->SizeInBytes() / Size(); }

    explicit CuckooFilter(const size_t max_num_keys) : num_items_(0), victim_(), hasher_()
    {
      size_t assoc = 4;
      size_t num_buckets = upperpower2(std::max<uint64_t>(1, max_num_keys / assoc));
      double frac = (double)max_num_keys / num_buckets / assoc;
      if (frac > 0.96)
      {
        //num_buckets <<= 1;
      }
      victim_.used = false;
      table_ = new TableType<bits_per_item>(num_buckets);
    }

    ~CuckooFilter() { delete table_; }

    // Add an item to the filter.
    Status Add(const ItemType &item);

    // Report if the item is inserted, with false positive rate.
    Status Contain(const ItemType &item) const;

    // Delete an key from the filter
    Status Delete(const ItemType &item);

    /* methods for providing stats  */
    // summary infomation
    std::string Info() const;

    // number of current inserted items;
    size_t Size() const { return num_items_; }

    // size of the filter in bytes.
    size_t SizeInBytes() const { return table_->SizeInBytes(); }
  };

  template <typename ItemType, size_t bits_per_item,
            template <size_t> class TableType, typename HashFamily>
  Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Add(
      const ItemType &item)
  {
    size_t i;
    uint32_t tag;

    if (victim_.used)
    {
      return NotEnoughSpace;
    }

    GenerateIndexTagHash(item, &i, &tag);
    return AddImpl(i, tag);
  }

  template <typename ItemType, size_t bits_per_item,
            template <size_t> class TableType, typename HashFamily>
  Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::AddImpl(
      const size_t i, const uint32_t tag)
  {
    size_t curindex = i;
    uint32_t curtag = tag;
    uint32_t oldtag;
    uint32_t tags[4];

    for (uint32_t count = 0; count < kMaxCuckooCount; count++)
    {
      bool kickout = count > 0;
      oldtag = 0;
      if (table_->InsertTagToBucket(curindex, curtag, kickout, oldtag, tags))
      {
        num_items_++;
        return Ok;
      }
      if (kickout)
      {
        curtag = oldtag;
      }
      curindex = AltIndex(curindex, curtag);
    }

    victim_.index = curindex;
    victim_.tag = curtag;
    victim_.used = true;
    return Ok;
  }

  template <typename ItemType, size_t bits_per_item,
            template <size_t> class TableType, typename HashFamily>
  Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Contain(
      const ItemType &key) const
  {
    bool found = false;
    size_t i1, i2;
    uint32_t tag;

    GenerateIndexTagHash(key, &i1, &tag);
    i2 = AltIndex(i1, tag);

    assert(i1 == AltIndex(i2, tag));

    found = victim_.used && (tag == victim_.tag) &&
            (i1 == victim_.index || i2 == victim_.index);

    if (found || table_->FindTagInBuckets(i1, i2, tag))
    {
      return Ok;
    }
    else
    {
      return NotFound;
    }
  }

  template <typename ItemType, size_t bits_per_item,
            template <size_t> class TableType, typename HashFamily>
  Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Delete(
      const ItemType &key)
  {
    size_t i1, i2;
    uint32_t tag;

    GenerateIndexTagHash(key, &i1, &tag);
    i2 = AltIndex(i1, tag);

    if (table_->DeleteTagFromBucket(i1, tag))
    {
      num_items_--;
      goto TryEliminateVictim;
    }
    else if (table_->DeleteTagFromBucket(i2, tag))
    {
      num_items_--;
      goto TryEliminateVictim;
    }
    else if (victim_.used && tag == victim_.tag &&
             (i1 == victim_.index || i2 == victim_.index))
    {
      // num_items_--;
      victim_.used = false;
      return Ok;
    }
    else
    {
      return NotFound;
    }
  TryEliminateVictim:
    if (victim_.used)
    {
      victim_.used = false;
      size_t i = victim_.index;
      uint32_t tag = victim_.tag;
      AddImpl(i, tag);
    }
    return Ok;
  }

  template <typename ItemType, size_t bits_per_item,
            template <size_t> class TableType, typename HashFamily>
  std::string CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Info() const
  {
    std::stringstream ss;
    ss << "CuckooFilter Status:\n"
       << "\t\t" << table_->Info() << "\n"
       << "\t\tKeys stored: " << Size() << "\n"
       << "\t\tLoad factor: " << LoadFactor() << "\n"
       << "\t\tHashtable size: " << (table_->SizeInBytes() >> 10) << " KB\n";
    if (Size() > 0)
    {
      ss << "\t\tbit/key:   " << BitsPerItem() << "\n";
    }
    else
    {
      ss << "\t\tbit/key:   N/A\n";
    }
    return ss.str();
  }
} // namespace cuckoofilter

#endif // CUCKOO_FILTER_CUCKOO_FILTER_H_
