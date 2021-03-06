#ifndef BFCASCADE_H
#define BFCASCADE_H

#include <cstdio>
#include <cstdint>
#include <cstring>

#include "cuckoo.h"

using namespace std;

template <typename fp_type, size_t fp_len>
class BFCascade {
	public :

    vector<BloomFilter<fp_type, fp_len>> bfc;
	uint64_t size_in_bytes = 0;
	size_t num_items_;

    ~BFCascade() {}

	void insert(vector<uint64_t> ins, vector<uint64_t> lup, vector<uint64_t> fp) { // , FILE *file)
		// max load factor of 95%
        double max_lf = 0.95;
        uint64_t init_size = ins.size() / max_lf;

		if (ins.size() > num_items_) // prevent overwriting during recursion
			num_items_ = ins.size();
        
        BloomFilter<uint16_t, 15> bloomFilter;
        bloomFilter.init(init_size, 1);
		/*
		if(bfc.size() & 1) { // odd = revoked
			cout << "bf contains REVOKED: level " << bfc.size() << endl;
		} 
		else { // even = unrevoked
			cout << "bf contains UNREVOKED: level " << bfc.size() << endl;
		}
		*/

	    for(uint64_t c : ins) { // int i = 0; i < int(ins.size()); i++
	        // uint64_t c = ins.at(i);
			// if(i == 0) {
			// 	cout << "index " << i << ": ";
			// 	cout << c << '\n';
			// }
	        bloomFilter.insert(c);
	    }
		bfc.push_back(bloomFilter);

	    // cout << "checking fp's: " << endl;
	    for(uint64_t l : lup) { // int j = 0; j < int(lup.size()); j++
	        // uint64_t l = lup.at(j);
	        if(bloomFilter.lookup(l)) {
	            fp.push_back(l);
				// cout << "fp: " << l << endl;
	        }
	    }
		double fpratio = (double) fp.size() / lup.size();
		size_in_bytes += bloomFilter.mem_cost();
		// cout << "# false hits: " << fp.size() << '\n';
		// cout << "fp: " << fpratio << '\n';
		// fprintf(file, "level, insert, lookup, # fp's, fp, memory(bytes), bits per item\n");
		printf("%lu, %lu, %lu, %lu, %.5f, %lu, %.5f\n", bfc.size(), ins.size(), lup.size(), fp.size(), fpratio, bloomFilter.mem_cost(), double(bloomFilter.mem_cost())/ins.size());
		// fprintf(file, "%lu, %lu, %lu, %lu, %.5f, %lu, %.5f\n", bfc.size(), ins.size(), lup.size(), fp.size(), fpratio, bloomFilter.mem_cost(), double(bloomFilter.mem_cost())/ins.size());
		// cout << "# fp's " << fp.size() << " fp " << fpratio << endl;

		if(fpratio == 1) {
			cout << "ERROR: fp = " << fpratio << endl;
			return;
		}

	    if(fp.size() > 0) { // RECURSION
			lup.clear();
	        insert(fp, ins, lup); 
	    }
		// cout << "final # levels: " << bfc.size() << endl;
		// cout << "insert " << ins.size() << " lookup " << lup.size() << " fp " << fp.size() << endl;
	}

	bool lookup(uint64_t e) { // true = revoked, false = unrevoked
		// cout << "lookup: " << e << endl;
		// cout << "lvls: " << bfc.size() << endl;
	    int l = 1; // track level
		for (BloomFilter<fp_type, fp_len> bf: bfc) {
	        // cout << "level " << l << ": ";
			// cout << "size " << it.size() << endl;
	        if (!bf.lookup(e)) {
	        	// cout << "not found" << endl;
	        	if (l & 1) { // odd # of levels
		        	// cout << "s (unrevoked)" << endl;
	        		return false;
	        	}
	        	else { // even
		        	// cout << "r (revoked)" << endl;
	        		return true;
	        	}
	        }
	        l++;
	    }
	    // cout << "in all bf's" << endl;
    	if(bfc.size() & 1) { // odd
        	// cout << "r (revoked)" << endl; // 4 levels, should NOT print
    		return true;
    	}
    	else { // even
        	// cout << "s (unrevoked)" << endl; // 5 levels, should NOT print
    		return false;
    	}
	}

    void print_cascade() {
	    cout << "\nprinting cascade stats...\n# of levels (bf's): " << bfc.size() << endl;
	    int i = 1;
		int total_size = 0;
		for (BloomFilter<fp_type, fp_len> bf: bfc) {
			total_size += bf.mem_cost();
	        cout << "\nlevel " << i << ": " << endl;
			// cout << "data? " << *(bf.table()) << endl;
			// auto *p = bf.table();
			// cout << "The vector elements are: "; 
			// for (int i = 0; i < int(bf.size()/8); ++i) 
			// 	cout << (int)*p++ << " "; 
			cout << "table size (bytes): " << bf.mem_cost() << endl;
			// cout << "element count: " << bf.element_count() << endl;
			// cout << "hash count: " << bf.hash_count() << endl;
			// cout << "effective fpp: " << bf.effective_fpp() << endl;
	        i++;
	    }
		cout << "total cascade size (bytes): " << total_size << "\n";
	}

	uint8_t num_levels() {
		return bfc.size();
	}

	uint64_t num_bytes() {
		return size_in_bytes;
	}

	double bits_per_item() {
		return 8.0 * size_in_bytes / num_items_;
	}
};

#endif