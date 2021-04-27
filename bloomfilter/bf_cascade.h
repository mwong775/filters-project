#ifndef BFCASCADE_H
#define BFCASCADE_H

#include <cstdio>
#include <cstdint>
#include <cstring>
// #include "BloomFilter.h"
#include "cuckoo.h"

using namespace std;

class BFCascade {
	public :

    vector<BloomFilter<uint16_t, 15>> bfc;
    ~BFCascade() {}

	void insert(vector<uint64_t> ins, vector<uint64_t> lup, vector<uint64_t> fp, FILE *file) {
		// max load factor of 95%
        double max_lf = 0.95;
        uint64_t init_size = ins.size() / max_lf;
        
        BloomFilter<uint16_t, 15> bloomFilter;
        bloomFilter.init(init_size, 1);
		if(bfc.size() & 1) { // odd = revoked
			cout << "bf contains REVOKED: level " << bfc.size() << endl;
		} 
		else { // even = unrevoked
			cout << "bf contains UNREVOKED: level " << bfc.size() << endl;
		}

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
		// cout << "# false hits: " << fp.size() << '\n';
		// cout << "fp: " << fpratio << '\n';
		// fprintf(file, "level, insert, lookup, # fp's, fp, memory(bytes), bits per item\n");
		fprintf(file, "%lu, %lu, %lu, %lu, %.5f, %lu, %.5f\n", bfc.size(), ins.size(), lup.size(), fp.size(), fpratio, bloomFilter.mem_cost(), double(bloomFilter.mem_cost())/ins.size());
		cout << "# fp's " << fp.size() << " fp " << fpratio << endl;

		if(fpratio == 1) {
			cout << "ERROR: fp = " << fpratio << endl;
			return;
		}

	    if(fp.size() > 0) {
			lup.clear();
	        insert(fp, ins, lup, file);
	    }
		// cout << "final # levels: " << bfc.size() << endl;
		// cout << "insert " << ins.size() << " lookup " << lup.size() << " fp " << fp.size() << endl;
	}

	bool lookup(uint64_t e) { // true = revoked, false = unrevoked
		// cout << "lookup: " << e << endl;
		// cout << "lvls: " << bfc.size() << endl;
	    int l = 1; // track level
		for (BloomFilter<uint16_t, 15> bf: bfc) {
	        // cout << "level " << l << ": ";
			// cout << "size " << it.size() << endl;
	        if(!bf.lookup(e)) {
	        	// cout << "not found" << endl;
	        	if(l & 1) { // odd # of levels
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
		for (BloomFilter<uint16_t, 15> bf: bfc) {
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
};

#endif