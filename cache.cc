#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <string.h>
#include "cache.h"

Cache::Cache (unsigned int l1_cache_size,unsigned int l1_block_size,unsigned int l1_assoc, unsigned int l2_cache_size, unsigned int l2_block_size, unsigned int l2_assoc,int current,unsigned int l1_victim_cache_no_of_blocks , unsigned int l2_victim_cache_no_of_blocks) {
	current_cache_no=current;
	cache_size=l1_cache_size;
	blocksize=l1_block_size;
	assoc=l1_assoc;
	int number_of_sets = cache_size / (blocksize*assoc);
	block_bit_width = log2 (blocksize);
	index_bit_width = log2 (number_of_sets);
	tag_bit_width = (int)32 - (block_bit_width + index_bit_width);
	//cout <<" tag_bit_width " << tag_bit_width << endl;
	data_to_AND_for_index = get_bits_to_multiply(block_bit_width, (index_bit_width+block_bit_width));
	data_to_AND_for_tag = get_bits_to_multiply((block_bit_width + index_bit_width) , (index_bit_width + block_bit_width + tag_bit_width));
	data_to_AND_for_index_block = get_bits_to_multiply(0, (block_bit_width + index_bit_width));
	data_to_AND_for_block = get_bits_to_multiply(block_bit_width , (index_bit_width + block_bit_width + tag_bit_width));
	//cout << "block_bit_width "<< block_bit_width << " index_bit_width "<< index_bit_width << " tag_bit_width " << tag_bit_width << endl;
	//cout << " data_to_AND_for_index " << data_to_AND_for_index << " data_to_AND_for_tag " << data_to_AND_for_tag << " data_to_AND_for_index_block " << data_to_AND_for_index_block << endl;
	vector<int> myRow(assoc,0);
	for (int i=0; i < number_of_sets; i++) {
		//cout << " I am here " << endl;
		data_cache.push_back(myRow);
		tag.push_back(myRow);
		valid.push_back(myRow);
		LRU.push_back(myRow);
		dirty.push_back(myRow);
	}
	if (l2_cache_size>0) {
		next_level_cache = new Cache(l2_cache_size, l2_block_size, l2_assoc, 0 , 0 , 0, 2, 0 ,0);
	} else {
		next_level_cache = NULL;
	}
	if (l1_victim_cache_no_of_blocks > 0) {
		unsigned int size = l1_victim_cache_no_of_blocks * l1_block_size;
		victim_cache = new Cache(size, l1_block_size , l1_victim_cache_no_of_blocks,0 , 0 , 0, 0 ,0,0);
	} else {
		//cout << "I am here" << endl;
		victim_cache = NULL;
	}
	//cout << " number_of_sets " << number_of_sets << endl;

	// initialize all statictics 
        read_count=0;
        read_miss_count=0;
        write_count=0;
        write_miss_count=0;
        mem_req_count=0;
	write_back_count=0;
	swap_request_count = 0;
	successful_swaps = 0;
}

unsigned int Cache::get_tag (unsigned int addr) {
	unsigned int temp;
	unsigned int my_tag;
        temp = data_to_AND_for_tag & addr;
	unsigned int shift = block_bit_width + index_bit_width;
	my_tag = temp >> shift;
	//cout << " data_to_AND_for_tag " << data_to_AND_for_tag << " addr " << addr << " my_tag " << my_tag << " shift " << shift << " block_bit_width " << block_bit_width << " index_bit_width " << index_bit_width << endl;
	//cout << " my_tag " << my_tag << endl;
	return my_tag;
}

unsigned int Cache::get_index (unsigned int addr) {
        
	unsigned int temp;
        unsigned int my_index;
        temp = data_to_AND_for_index & addr;
        //cout << "data_to_AND_for_index " << data_to_AND_for_index << endl;
	my_index = temp >> block_bit_width;

	//unsigned int<5> x;
	//cout << std::hex << ((1 << (index_bit_width+1))-1) << endl;
	//cout << " my_index " << my_index << endl;
	return my_index;

}



unsigned int Cache::get_block_address (unsigned int addr) {

	unsigned int temp;
        unsigned int my_block_address;
        temp = data_to_AND_for_block & addr;
        //cout << "data_to_AND_for_index " << data_to_AND_for_index << endl;
        my_block_address = temp >> block_bit_width;
	my_block_address = my_block_address * pow(2,block_bit_width);
        //unsigned int<5> x;
        //cout << std::hex << ((1 << (index_bit_width+1))-1) << endl;
        //cout << " my_index " << my_index << endl;
        return my_block_address;


}
unsigned int Cache::get_bits_to_multiply (int start, int end) {

	unsigned int data_to_mult = 0;
	unsigned int temp = 0;
	for (int i=start ; i< end; i++ ) {
		temp = pow(2,i);
		data_to_mult = data_to_mult + temp;
	}
	return data_to_mult;

}

bool Cache::is_hit (unsigned int addr) {

	//cout << "index_to_access " << index_to_access << endl;
	unsigned int tag_to_access = get_tag(addr);
	unsigned int index_to_access = get_index(addr);
	//cout << " tag " << tag_to_access << " index " << index_to_access << " size " << data_cache[0].size() << endl;
	for (int j = 0; j < tag[index_to_access].size(); j++) {
		//cout << tag[index_to_access][j] << endl;
		if ( tag[index_to_access][j] == tag_to_access && valid[index_to_access][j]==1) {
			//Edit
			//cout << " L" << current_cache_no <<" hit" << endl;
			return true;
		
		}

	}
	
	/*for ( vector<int>::iterator it = tag[index_to_access].begin(); it != tag[index_to_access].end(); ++ it ) 
	{
		cout << " tag " << *it << endl;
		cout << "I am here" << endl;
	}*/
	//Edit
	//cout << " L" << current_cache_no << " miss" << endl;
	return false;

}

void Cache::read_data (unsigned int addr) {
        unsigned int tag_to_access = get_tag(addr);
        unsigned int index_to_access = get_index(addr);
	unsigned int mru_position;
	bool block_replaced_in_victim = false;
	unsigned int my_block_address = get_block_address(addr);
	//Edit
	//cout << " L" << current_cache_no << " read: " << std::hex << my_block_address << " (tag " << std::hex << tag_to_access << ", index " << std::dec << index_to_access << ")" << endl;	
	bool hit = is_hit(addr);
	// increment read count 
	read_count++;
	//cout << " L1 victim: none " << endl;
	//bin(data_to_AND_for_index_block & addr);
	//cout << endl;
	//cout << "tag_to_access :: " << std::hex << tag_to_access << " index_to_access:: " << index_to_access << endl;
	//cout << " I am here " << endl;
	if (hit == true) {
		// for hit, get the position of the data to update lru array
		//cout << "Cache hit :: address " << endl;
	        for (int j = 0; j < tag[index_to_access].size(); j++) {
                	if ( (tag[index_to_access][j] == tag_to_access) && (valid[index_to_access][j]==1) ) {
                        	mru_position = j;
				break;
                	}
		//cout << "position of the block ::" << mru_position << endl; 

        	}
	} else {
		//cout << "Cache miss :: address" << endl;
		// for miss, get the position of the data to evict
		read_miss_count++;
		int least_ru =0;
		int block_to_replace = 0;
		int j=0;
		for (j = 0; j < LRU[index_to_access].size(); j++) {
                	if (least_ru < LRU[index_to_access][j]) {
                        	block_to_replace = j;
				least_ru = LRU[index_to_access][j];
                		//cout << LRU[index_to_access][j] << "  " ;
			}
			//cout << LRU[index_to_access][j] << "  " ;
        	}
                if (valid[index_to_access][block_to_replace]==1) {
                        if (dirty[index_to_access][block_to_replace]==1) {
                        	//Edited
				//cout << " L" << current_cache_no <<" victim: "<< std::hex << data_cache[index_to_access][block_to_replace] << " (tag " << std::hex << get_tag(data_cache[index_to_access][block_to_replace]) << ", index " << std::dec << index_to_access << ", dirty)" << endl;

                        } else {
                                //Edited
				//cout << " L"<< current_cache_no <<" victim: "<< std::hex << data_cache[index_to_access][block_to_replace] << " (tag " << std::hex << get_tag(data_cache[index_to_access][block_to_replace]) << ", index " << std::dec << index_to_access << ", clean)" << endl;
                        }
                } else {
			//Edited
			//cout << " L" << current_cache_no <<" victim: none"<< endl;
		}
		// if the bit is dirty write to next level
		// searching victim cache
		if (victim_cache!=NULL) {
			if (valid[index_to_access][block_to_replace]==1) { 
				//Edited
				//cout << " VC swap req: [" << hex << my_block_address << ", "<< hex << data_cache[index_to_access][block_to_replace] <<"]" << endl;
				swap_request_count++;
				if (search_victim_cache(my_block_address)) {
					//cout << " Inserting block " << data_cache[index_to_access][block_to_replace] << " is dirty " << dirty[index_to_access][block_to_replace] << endl;
					replace_data_victim_cache(index_to_access, block_to_replace,block_to_replace_in_victim );
					block_replaced_in_victim = true;
					successful_swaps++;
				} else 	{
					if (valid[index_to_access][block_to_replace] == 1) {
					//cout << " Inserting block " << data_cache[index_to_access][block_to_replace] << " is dirty " << dirty[index_to_access][block_to_replace] << endl;
						insert_data_victim_cache(data_cache[index_to_access][block_to_replace], dirty[index_to_access][block_to_replace]);
					}
				}
			}
		}
		if (victim_cache==NULL && dirty[index_to_access][block_to_replace]==1) {
			write_back_count++;
		}
		//
		if (dirty[index_to_access][block_to_replace] == 1) {
			if (next_level_cache!=NULL && victim_cache==NULL) {
				//cout << " In the next level " << endl;
				//need to write the data to be replaced in L2 cache if the cache block is dirty
				next_level_cache->write_data(data_cache[index_to_access][block_to_replace]);
				//cout << " returned " << endl;
			}
		}
                if (next_level_cache!=NULL && block_replaced_in_victim==false) {
                        //cout << " in the next level " << endl;
                        next_level_cache->read_data(addr);
                        //cout << " returned "; 
                }
		if (block_replaced_in_victim==false) {
			valid[index_to_access][block_to_replace] = 1;
			tag[index_to_access][block_to_replace] = tag_to_access;
			data_cache[index_to_access][block_to_replace] = my_block_address;
			dirty[index_to_access][block_to_replace] = 0;
			mem_req_count++;
		}
		mru_position = block_to_replace;
		//cout << "block evicted :: " << mru_position << endl;

	}
	update_LRU(mru_position , index_to_access);
	//cout << "printing tag" << endl;
	/*for(int i=0; i < tag.size(); i++) {
		for(int a=0; a < tag[i].size() ; a++) {
			cout << tag[i][a] << " ";
	
		}
		cout << endl;
	}

	cout << "printing LRU" << endl;
        for(int i=0; i < LRU.size(); i++) {
                for(int a=0; a < LRU[i].size() ; a++) {
                        cout << LRU[i][a] << " ";

                }
                cout << endl;
        }*/

}


void Cache::write_data (unsigned int addr) {
        unsigned int tag_to_access = get_tag(addr);
        unsigned int index_to_access = get_index(addr);
        unsigned int mru_position;
	bool is_dirty=false;
	bool block_replaced_in_victim = false;
        unsigned int my_block_address = get_block_address(addr);
	//Edited
	//cout << " L"<< current_cache_no <<" write: " << std::hex << my_block_address << " (tag " << std::hex << tag_to_access << ", index " << std::dec << index_to_access << ")" << endl;
        //unsigned int tag_to_access = get_tag(addr);
        bool hit = is_hit(addr);
	//cout << " L1 victim: none " << endl;
	//bin(data_to_AND_for_index_block & addr);
        //cout << endl;
	//increament write counr
	write_count++;
	//cout << "tag_to_access :: " << std::hex << tag_to_access << " index_to_access:: " << index_to_access << endl;
        if (hit == true) {
                // for hit, get the position of the data to update lru array
                //cout << "Cache hit :: address " << endl;
                for (int j = 0; j < tag[index_to_access].size(); j++) {
                        if ( (tag[index_to_access][j] == tag_to_access) && (valid[index_to_access][j]==1) ) {
                                mru_position = j;
				// set dirty bit 
				dirty[index_to_access][j]=1;
                                is_dirty=true;
				break;
                        }
                //cout << "position of the block ::" << mru_position << endl;

                }
        } else {
		//cout << "Cache miss :: address" << endl;
                // for miss, get the position of the data to evict
		write_miss_count++;
                int least_ru =0;
                int block_to_replace = 0;
                int j=0;
                for (j = 0; j < LRU[index_to_access].size(); j++) {
                        if (least_ru < LRU[index_to_access][j]) {
                                block_to_replace = j;
				least_ru = LRU[index_to_access][j];
                        }
                }
                if (valid[index_to_access][block_to_replace]==1) {
                        if (dirty[index_to_access][block_to_replace]==1) {
                        	//Edited
				//cout << " L" << current_cache_no << " victim: "<< std::hex << data_cache[index_to_access][block_to_replace] << " (tag " << std::hex << get_tag(data_cache[index_to_access][block_to_replace]) << ", index " << std::dec << index_to_access << ", dirty)" << endl;

                        } else {
                                //Edited
				//cout << " L" << current_cache_no <<" victim: "<< std::hex << data_cache[index_to_access][block_to_replace] << " (tag " << std::hex << get_tag(data_cache[index_to_access][block_to_replace]) << ", index " << std::dec << index_to_access << ", clean)" << endl;
                        }
		} else {
			//Edited
			//cout << " L" << current_cache_no <<" victim: none"<< endl; 
		}
		// searching victim cache
		if (victim_cache!=NULL) {
			if (valid[index_to_access][block_to_replace]==1) {
			        swap_request_count++;	
				//Edited
				//cout << " VC swap req: [" << hex << my_block_address << ", "<< hex << data_cache[index_to_access][block_to_replace] <<"]" << endl;
                		if (search_victim_cache(my_block_address)) {
					//cout << " Inserting block " << data_cache[index_to_access][block_to_replace] << " is dirty " << dirty[index_to_access][block_to_replace] << endl;
                        		replace_data_victim_cache(index_to_access, block_to_replace,block_to_replace_in_victim );
                			block_replaced_in_victim = true;
					successful_swaps++;
				} else {
					if (valid[index_to_access][block_to_replace] == 1) {
						//cout << " Inserting block " << data_cache[index_to_access][block_to_replace] << " is dirty " << dirty[index_to_access][block_to_replace] << endl;
                        			insert_data_victim_cache(data_cache[index_to_access][block_to_replace], dirty[index_to_access][block_to_replace]);
					}
                		}
			}
		}
		/////
		if (victim_cache==NULL && dirty[index_to_access][block_to_replace]==1) {
                        write_back_count++;
                }

		//VC swap req: [423aeba0, 400341a0]
                if (dirty[index_to_access][block_to_replace] == 1) {
                        if (next_level_cache!=NULL && victim_cache==NULL) {
                                //cout << " In the next level" << endl;
				//need to write the data to be replaced in L2 cache if the cache block is dirty
                                next_level_cache->write_data(data_cache[index_to_access][block_to_replace]);
                                //cout << " returned " << endl;
                        }
                }

		if (next_level_cache!=NULL && block_replaced_in_victim == false) {
                        //cout << " In the next level" << endl;
                        next_level_cache->read_data(addr);
                        //cout << " returned " << endl;
                }
		if (block_replaced_in_victim == false) {
			mem_req_count++;
		}	
                valid[index_to_access][block_to_replace] = 1;
                tag[index_to_access][block_to_replace] = tag_to_access;
		data_cache[index_to_access][block_to_replace] = my_block_address;
                mru_position = block_to_replace;
		dirty[index_to_access][block_to_replace]=1;
                is_dirty=true;
		//cout << "block evicted :: " << mru_position << endl;

        }
        update_LRU(mru_position , index_to_access);
	if (is_dirty==true) {
		//Edited
		//cout << " L" << current_cache_no << " set dirty" << endl;
	}

}


void Cache::update_LRU(unsigned int mru_position ,unsigned int index_to_access) {
	bool LRU_updated=false;	
	int temp = LRU[index_to_access][mru_position];
	for (int j = 0; j < LRU[index_to_access].size(); j++) {
		if (j == mru_position ) {
			LRU[index_to_access][j] = 0;
			LRU_updated=true;
		} else if (LRU[index_to_access][j] <= temp) {
			LRU[index_to_access][j]++;
			LRU_updated=true;
			//cout << " L1 update LRU " << endl;
		}
	}
	if (LRU_updated) {
		//Edited
		//cout << " L" << current_cache_no << " update LRU" << endl;
	}

}

void Cache::update_cache_on_read_miss (unsigned int tag_to_access, unsigned int index_to_access) {
	
	// find a block to evict
	unsigned int block_to_replace = 0;
	unsigned int least_ru = 0;
	for (int j = 0; j < LRU[index_to_access].size(); j++) {
		if (least_ru < LRU[index_to_access][j]) {
			block_to_replace = j;
		}
	}


}

void Cache::print_cache_contents() {
	cout << endl;
	cout << "===== " << "L" << current_cache_no << " contents =====" << endl;
        for(int i=0; i < tag.size(); i++) {
		cout <<" set   " << dec << i <<": ";
		int b = tag[i].size();
		int current_lru = 0;
		while (b>0) {
                	for(int a=0; a < tag[i].size() ; a++) {
				// print the elements in the ascending order of LRU values
				if (LRU[i][a]==current_lru) {
					if (dirty[i][a]==1) {
                        			cout << std::hex << tag[i][a] <<" D";
        				} else {
						cout << hex << tag[i][a] <<"  ";
					}
					current_lru++;
					break;
				}
                	}
			b--;
		}
                cout << endl;
        }
	if (victim_cache!=NULL) {
		print_victim();
	}
	if (next_level_cache!=NULL) {
		next_level_cache->print_cache_contents();
	}
}


// victim cache
void Cache::print_victim() {
	cout << endl;
	cout <<"===== VC contents =====" << endl;
	int b = victim_cache->tag[0].size();
	int current_lru = 0;
	cout <<"set 0:" << "  ";
	while (b>0) {
		for (int j = 0; j < victim_cache->tag[0].size(); j++) {
			if (victim_cache->LRU[0][j]==current_lru) {
				cout << victim_cache->tag[0][j] << "   "; 
				if (victim_cache->dirty[0][j]==1) {
					cout << " D "  ;
				}
				current_lru++;
				break;
			}
			
		}		
		b--;
	}
	cout << endl;
}


bool Cache::search_victim_cache(unsigned int addr) {

	//cout << "index_to_access " << index_to_access << endl;
        //unsigned int tag_to_access = victim_cache->get_tag(addr);
        //unsigned int index_to_access = victim_cache->get_index(addr);
        //cout << " tag " << tag_to_access << " index " << index_to_access << " size " << data_cache[0].size() << endl;
        for (int j = 0; j < victim_cache->tag[0].size(); j++) {
                //cout << tag[index_to_access][j] << endl;
                if ( victim_cache->data_cache[0][j] == addr && victim_cache->valid[0][j]==1) {
                        if (victim_cache->dirty[0][j]==1) {
				// Edit
				//cout << " VC hit: " << victim_cache->data_cache[0][j] << " (dirty)" <<endl;
			} else {
				//Edit
				//cout << " VC hit: " << victim_cache->data_cache[0][j] << " (clean)" <<endl;
			}
			block_to_replace_in_victim = j;
			return true;

                }
        }
	//Edit
	//cout << " VC miss " << endl;
	return false;
}

void Cache::replace_data_victim_cache(unsigned int my_index_cache, unsigned int block_position, unsigned int block_to_replace_in_victim ) {
	
	unsigned int addr_cache= data_cache[my_index_cache][block_position];
	unsigned int dirty_cache= dirty[my_index_cache][block_position];
	unsigned int victim_addr = victim_cache->data_cache[0][block_to_replace_in_victim];
	data_cache[my_index_cache][block_position] = victim_addr;
	tag[my_index_cache][block_position] = get_tag(victim_addr);
	dirty[my_index_cache][block_position] = victim_cache->dirty[0][block_to_replace_in_victim];
	victim_cache->data_cache[0][block_to_replace_in_victim] = victim_cache->get_block_address(addr_cache);
	//cout << "setting dirty in_replace" << dirty_cache << endl;
	victim_cache->dirty[0][block_to_replace_in_victim] = dirty_cache;
	victim_cache->tag[0][block_to_replace_in_victim] = victim_cache->get_tag(addr_cache);
	victim_cache_update_LRU(block_to_replace_in_victim);
}

void Cache::insert_data_victim_cache(unsigned int addr,int dirty) {
	int least_ru = 0;
	int block_to_replace=0;
	for (int i=0; i < victim_cache->data_cache[0].size() ; i++) {
           if (least_ru < victim_cache->LRU[0][i]) {
                       block_to_replace = i;
                       least_ru = victim_cache->LRU[0][i];
               }
        }
	if (victim_cache->valid[0][block_to_replace]==1) {
                if (victim_cache->dirty[0][block_to_replace]==1) {
                        //Edit
			//cout << " VC victim: "<< std::hex << victim_cache->data_cache[0][block_to_replace] << " (tag " << std::hex << victim_cache->tag[0][block_to_replace] << ", index 0, dirty)" << endl;
                        write_back_count++;
			if (next_level_cache!=NULL ) {
                                //cout << " In the next level" << endl;
                                //need to write the data to be replaced in L2 cache if the cache block is dirty
                                next_level_cache->write_data(victim_cache->data_cache[0][block_to_replace]);
                                //cout << " returned " << endl;
                        	}
                        } else {
                        	//Edit
				//cout << " VC victim: "<< std::hex << victim_cache->data_cache[0][block_to_replace] << " (tag " << std::hex << victim_cache->tag[0][block_to_replace] << ", index 0, clean)" << endl;
                        }
        } else {
                //Edit
		//cout << " VC victim: none"<< endl;
        }
	//cout <<" block_to_replace " << block_to_replace << " LRU values " << victim_cache->LRU[0][0] << " " << victim_cache->LRU[0][1] << endl;
	victim_cache->data_cache[0][block_to_replace]=victim_cache->get_block_address(addr);
	victim_cache->tag[0][block_to_replace]=victim_cache->get_tag(addr);
	//cout << "setting dirty in_insert_data " << dirty << endl;
	victim_cache->dirty[0][block_to_replace]=dirty;
	victim_cache->valid[0][block_to_replace] = 1;
	//LRU update for victim cache
	victim_cache_update_LRU(block_to_replace);
	//print_victim();
}
void Cache::victim_cache_update_LRU(int block_to_replace) {
	bool LRU_updated=false;	
        int temp = victim_cache->LRU[0][block_to_replace];
	//cout << " temp " << temp << "size "<< LRU[0].size() << endl;
        for (int j = 0; j < victim_cache->LRU[0].size(); j++) {
                if (j == block_to_replace ) {
                        victim_cache->LRU[0][j] = 0;
			LRU_updated=true;
                } else if ((victim_cache->LRU[0][j]) <= temp) {
			//cout << " I am here " << endl;
                        victim_cache->LRU[0][j] = victim_cache->LRU[0][j] + 1;
                        LRU_updated=true;
                        //cout << " L1 update LRU " << endl;
                }
		//cout << "LRU value "<< victim_cache->LRU[0][j] << endl;
        }
	//LRU updated
        if (LRU_updated) {
                //Edit
		//cout << " VC update LRU" << endl;
        }


}

void Cache::print_stat() {
       //cout << "a. number of L" << current_cache_no << " reads:\t" << dec << read_count << endl; 
       //cout << "b. number of L" << current_cache_no << " read misses:\t" << dec << read_miss_count << endl; 
       //cout << "c. number of L" << current_cache_no << " writes:\t" << dec << write_count << endl; 
       //cout << "d. number of L" << current_cache_no << " write misses:\t" << dec << write_miss_count << endl; 
       if (current_cache_no==1) {
       		cout << "a. number of L" << current_cache_no << " reads:\t" << dec << read_count << endl;
       		cout << "b. number of L" << current_cache_no << " read misses:\t" << dec << read_miss_count << endl;
       		cout << "c. number of L" << current_cache_no << " writes:\t" << dec << write_count << endl;
       		cout << "d. number of L" << current_cache_no << " write misses:\t" << dec << write_miss_count << endl;
	        cout << "e. number of swap requests:\t" << dec << swap_request_count << endl;
	        float swap_request_rate = (float) swap_request_count / ((float) read_count + (float) write_count);
		//cout << "f. swap request rate: \t" << dec << (float) swap_request_count / ((float) read_count + (float) write_count) << endl; 
	        printf ("f. swap request rate: %0.4f\n", swap_request_rate);
	        cout << "g. number of swaps:\t" << dec << successful_swaps << endl;
	       	//cout << "h. combined L1+VC miss rate:\t" << dec << ((float) read_miss_count + (float) write_miss_count - (float) successful_swaps) / ((float) read_count + (float) write_count) << endl; 
	       float l1_miss_rate = ((float) read_miss_count + (float) write_miss_count - (float) successful_swaps) / ((float) read_count + (float) write_count);
	       printf("h. combined L1+VC miss rate:   %0.4f \n", l1_miss_rate);
	       cout << "i. number writebacks from L1/VC:\t" << dec << write_back_count << endl;
       } else {
	       	cout << "j. number of L" << current_cache_no << " reads:\t" << dec << read_count << endl;
       		cout << "k. number of L" << current_cache_no << " read misses:\t" << dec << read_miss_count << endl;
       		cout << "l. number of L" << current_cache_no << " writes:\t" << dec << write_count << endl;
       		cout << "m. number of L" << current_cache_no << " write misses:\t" << dec << write_miss_count << endl;
       		//cout << "n. L2 miss rate:\t" << dec << ((float) read_miss_count + (float) write_miss_count) / ((float) read_count + (float) write_count) << endl;	
	       	float l2_miss_rate = ((float) read_miss_count) / ((float) read_count);
		printf(" n. L2 miss rate:  %0.4f \n", l2_miss_rate);
		cout << "o. number of writebacks from L2:\t" << dec << write_back_count << endl;
       }
	if (next_level_cache!=NULL) {
		next_level_cache->print_stat(); 
			
	} else {
	       	if (current_cache_no==1) {
			cout << "j. number of L2 reads:  0 "<< endl;
       	       		cout << "k. number of L2 read misses:\t 0" << endl;
       	       		cout << "l. number of L2 writes:\t 0" << endl;
       	       		cout << "m. number of L2 write misses:\t 0" << endl;
	       		cout << "n. L2 miss rate:\t 0.0000" << endl;
       	       		cout << "o. number of writebacks from L2:\t   0" << endl;
 		}		
       	       	cout << "p. total memory traffic:\t" << dec << mem_req_count+write_back_count << endl;
	}	

	
}

