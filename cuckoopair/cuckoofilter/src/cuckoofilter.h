#ifndef CUCKOO_FILTER_CUCKOO_FILTER_H_
#define CUCKOO_FILTER_CUCKOO_FILTER_H_

#include <assert.h>
#include <algorithm>

#include "debug.h"
#include "hashutil.h"
#include "packedtable.h"
#include "printutil.h"
#include "singletable.h"

namespace cuckoofilter {
// status returned by a cuckoo filter operation
enum Status {
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
class CuckooFilter {
  // Storage of items
  TableType<bits_per_item> *table_;

  // Number of items stored
  size_t num_items_;

  typedef struct {
    size_t index;
    uint32_t tag;
    bool used;
  } VictimCache;

  VictimCache victim_;

  HashFamily hasher_;

  std::vector<uint8_t> seeds_;

  template <typename K>
  inline uint64_t Hash(const K &key, uint32_t seed = 0) const {
    return hasher_(key, seed);
  }

  inline size_t IndexHash(const ItemType &item) const {
    // table_->num_buckets is always a power of two, so modulo can be replaced
    // with
    // bitwise-and:
    const uint32_t hash = item >> 32;
    return hash & (table_->NumBuckets() - 1);
  }

  inline uint32_t TagHash(
      uint64_t hv) const {  // equiv. to partial_key function in hashtable
    uint32_t tag;
    tag = hv & ((1ULL << bits_per_item) - 1);
    tag += (tag == 0);
    return tag;
  }

  inline void GenerateIndexTagHash(const ItemType &item, size_t *index,
                                   uint32_t *tag) const {
    *index = IndexHash(item);  // original: hash >> 32
    // if (seeds_.at(*index) > 0)
    //   std::cout << "rehashed " << seeds_.at(*index) << " bucket " << *index
    //             << "\n";
    const uint64_t hash = hasher_(item, seeds_.at(*index));
    *tag = TagHash(hash);
  }

  // modified of above function to use for 2 indices and tags
  inline void GenerateTagHashes(const ItemType &item, size_t *i1, size_t *i2,
                                uint32_t *tag1, uint32_t *tag2) const {
    *i1 = IndexHash(item);  // original: hash >> 32
    *i2 = AltIndex(*i1, item);
    const uint64_t hash1 = hasher_(item, seeds_.at(*i1));
    const uint64_t hash2 = hasher_(item, seeds_.at(*i2));
    *tag1 = TagHash(hash1);
    *tag2 = TagHash(hash2);
  }

  /*
  inline size_t AltIndex(const size_t index, const uint32_t tag) const {
    // NOTE(binfan): originally we use:
    // index ^ HashUtil::BobHash((const void*) (&tag), 4)) & table_->INDEXMASK;
    // now doing a quick-n-dirty way:
    // 0x5bd1e995 is the hash constant from MurmurHash2
    return IndexHash((uint32_t)(index ^ (tag * 0x5bd1e995)));
  }
  */
  inline size_t AltIndex(const size_t index, const ItemType &item) const {
    // NOTE(binfan): originally we use:
    // index ^ HashUtil::BobHash((const void*) (&tag), 4)) & table_->INDEXMASK;
    // now doing a quick-n-dirty way:
    // 0x5bd1e995 is the hash constant from MurmurHash2
    const size_t hp = log2(table_->NumBuckets());
    const size_t fp = (item >> hp) + 1;
    const size_t hashmask = table_->NumBuckets() - 1;
    // return IndexHash((uint32_t)(index ^ (item * 0x5bd1e995)));
    return (index ^ (fp * 0xc6a4a7935bd1e995)) & hashmask;
  }

  Status AddImpl(const size_t i, const uint32_t tag);

  // load factor is the fraction of occupancy
  double LoadFactor() const { return 1.0 * Size() / table_->SizeInTags(); }

  double BitsPerItem() const { return 8.0 * table_->SizeInBytes() / Size(); }

 public:
  explicit CuckooFilter(const size_t max_num_keys)
      : num_items_(0), victim_(), hasher_() {
    size_t assoc = 4;
    size_t num_buckets =
        upperpower2(std::max<uint64_t>(1, max_num_keys / assoc));
    double frac = (double)max_num_keys / num_buckets / assoc;
    if (frac > 0.96) {
      num_buckets <<= 1;
    }
    victim_.used = false;
    table_ = new TableType<bits_per_item>(num_buckets);
  }

  // modified constructor
  explicit CuckooFilter(const size_t max_num_keys,
                        const std::vector<uint8_t> &seeds)
      : num_items_(0), victim_(), hasher_() {
    size_t assoc = 4;
    size_t num_buckets = seeds.size();
    // upperpower2(std::max<uint64_t>(1, max_num_keys / assoc));
    // double frac = (double)max_num_keys / num_buckets / assoc;
    // if (frac > 0.96) {
    //   num_buckets <<= 1;
    // }
    victim_.used = false;
    seeds_ = seeds;
    // std::cout << "init filter seeds: [ ";
    // for (auto i : seeds_) {
    //   std::cout << i << " ";
    // }
    // std::cout << "]\nseeds array size: " << seeds_.size() << "\n";
    table_ = new TableType<bits_per_item>(num_buckets);
  }

  ~CuckooFilter() { delete table_; }

  // Add an item to the filter.
  Status Add(const ItemType &item);

  // Insert item to the filter at given bucket index and slot.
  Status CopyInsert(const uint32_t fp, size_t index, size_t slot);

  // Report if the item is inserted, with false positive rate.
  Status Contain(const ItemType &item) const;

  // Delete an key from the filter
  Status Delete(const ItemType &item);

  size_t SeedTable_Size() const;

  /* methods for providing stats  */
  // summary infomation
  std::string Info() const;

  // number of current inserted items;
  size_t Size() const { return num_items_; }

  // size of the filter in bytes. TODO: modify to include seeds if used.
  size_t SizeInBytes() const { return table_->SizeInBytes(); }
};

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
Status CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::Add(
    const ItemType &item) {
  size_t i;
  uint32_t tag;

  if (victim_.used) {
    return NotEnoughSpace;
  }

  GenerateIndexTagHash(item, &i, &tag);
  return AddImpl(i, tag);
}

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
Status CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::AddImpl(
    const size_t i, const uint32_t tag) {
  size_t curindex = i;
  uint32_t curtag = tag;
  uint32_t oldtag;

  for (uint32_t count = 0; count < kMaxCuckooCount; count++) {
    bool kickout = count > 0;
    oldtag = 0;
    if (table_->InsertTagToBucket(curindex, curtag, kickout, oldtag)) {
      num_items_++;
      return Ok;
    }
    if (kickout) {
      curtag = oldtag;
    }
    curindex = AltIndex(curindex, curtag);
  }

  victim_.index = curindex;
  victim_.tag = curtag;
  victim_.used = true;
  return Ok;
}

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
Status CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::CopyInsert(
    const uint32_t fp, const size_t index, const size_t slot) {
  if (table_->CopyTagToBucket(index, slot, fp)) {
    num_items_++;
    // std::cout << "copied " << fp << " to " << index << "\n";
    return Ok;
  }
  return NotSupported;
}

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
Status CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::Contain(
    const ItemType &key) const {
  bool found = false;
  size_t i1, i2;
  uint32_t tag1, tag2;

  GenerateTagHashes(key, &i1, &i2, &tag1, &tag2);
  assert(i1 == AltIndex(i2, key));

  // std::cout << "lup " << tag1 << ", " << tag2 << ": " << i1 << ", " << i2
  //           << "\n";

  // found = victim_.used && (tag1 == victim_.tag || tag2 == victim_.tag) &&
  //         (i1 == victim_.index || i2 == victim_.index);

  if (table_->FindTagInBuckets(i1, i2, tag1, tag2)) {  // found ||
    return Ok;
  } else { // WARNING: only use for checking false negative's (if not, get spammed D:)
   
    // std::cout << "failed to find " << key << " at " << i1 // << ", " << i2
    //           << "\n\t\thv: " << Hash(key, seeds_.at(i1)) << " " // << " (" << hasher_(key, seeds_.at(i1)) << ") "
    //           // << Hash(key, seeds_.at(i2)) //<< " (" << hasher_(key, seeds_.at(i2)) << ") " 
    //           << "\n\t\tfp " << tag1 // << " " << tag2
    //           << "\n\t\tseed: " << seeds_.at(i1) << "\n"; // << " and " << seeds_.at(i2) << "\n";
    // table_->PrintBucket(i1);
   
    //   if (j < 3) std::cout << ", ";
    // }
    // std::cout << "\tseed: " << seeds_.at(i1) << "\n";
    // table_->PrintBucket(i2);
    // std::cout << "\tseed: " << seeds_.at(i2) << "\n";
    return NotFound;
  }
   // std::cout << "HV's : [";
    // for (size_t j = 0; j < 4; j++) {
    //   if (table_->ReadTag(i1, j) != 0)
    //     std::cout << Hash(key, seeds_.at(i1));
    //   else
    //     std::cout << " ";
}

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
Status CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::Delete(
    const ItemType &key) {
  size_t i1, i2;
  uint32_t tag;

  GenerateIndexTagHash(key, &i1, &tag);
  i2 = AltIndex(i1, tag);

  if (table_->DeleteTagFromBucket(i1, tag)) {
    num_items_--;
    goto TryEliminateVictim;
  } else if (table_->DeleteTagFromBucket(i2, tag)) {
    num_items_--;
    goto TryEliminateVictim;
  } else if (victim_.used && tag == victim_.tag &&
             (i1 == victim_.index || i2 == victim_.index)) {
    // num_items_--;
    victim_.used = false;
    return Ok;
  } else {
    return NotFound;
  }
TryEliminateVictim:
  if (victim_.used) {
    victim_.used = false;
    size_t i = victim_.index;
    uint32_t tag = victim_.tag;
    AddImpl(i, tag);
  }
  return Ok;
}

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
size_t CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::SeedTable_Size() const {
  return sizeof(std::vector<uint8_t>) + (sizeof(uint8_t) * seeds_.size());
}

template <typename ItemType, size_t bits_per_item, typename HashFamily,
          template <size_t> class TableType>
std::string CuckooFilter<ItemType, bits_per_item, HashFamily, TableType>::Info()
    const {
  std::stringstream ss;
  size_t table_size = table_->SizeInBytes();
  size_t seedtable_size = SeedTable_Size();
  size_t total_size = table_size + seedtable_size;
  ss << "CuckooFilter Status:\n"
     << "\t\t" << table_->Info()
     << "\n"
     //  << "\t\tNumBuckets: " << table_->NumBuckets() << "\n" // same as "Total
     //  # of rows"
     << "\t\tKeys stored: " << Size() << "\n"
     << "\t\tLoad factor: " << LoadFactor() << "\n"
     << "\t\tHashtable size: " 
     << table_size << " (table) + " << seedtable_size << " (seeds) = " << total_size << " bytes ("
     << (table_size >> 10) << " KB)\n";
  if (Size() > 0) {
    ss << "\t\tbit/key:   " << BitsPerItem() << "\n";
  } else {
    ss << "\t\tbit/key:   N/A\n";
  }
  return ss.str();
}
}  // namespace cuckoofilter
#endif  // CUCKOO_FILTER_CUCKOO_FILTER_H_
