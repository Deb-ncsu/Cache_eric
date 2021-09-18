#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <string.h>

using namespace std;
class Cache {
        public:
                int cache_size;
                int blocksize;
                int assoc;
                int block_bit_width;
                int index_bit_width;
                int tag_bit_width;
                int current_cache_no;
                unsigned int data_to_AND_for_index;
                unsigned int data_to_AND_for_tag;
                unsigned int data_to_AND_for_index_block;
                unsigned int data_to_AND_for_block;
                Cache * next_level_cache;
                Cache * victim_cache;
                vector< vector<int> > data_cache;
                vector< vector<int> > tag;
                vector< vector<int> > valid;
                vector< vector<int> > LRU;
                vector< vector<int> > dirty;
                Cache(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, int, unsigned int, unsigned int);
                unsigned int block_to_replace_in_victim;
                unsigned int get_tag(unsigned int);
                unsigned int get_index(unsigned int);
                bool is_hit(unsigned int);
                unsigned int get_bits_to_multiply (int start, int end);
                void read_data (unsigned int);
                void update_cache_on_read_miss (unsigned int, unsigned int);
                void update_LRU (unsigned int, unsigned int);
                void bin(unsigned );
                void write_data (unsigned int);
                unsigned int get_block_address(unsigned int);
                void print_cache_contents();
                bool search_victim_cache(unsigned int );
                void replace_data_victim_cache(unsigned int , unsigned int , unsigned int );
                void insert_data_victim_cache(unsigned int ,int );
                void victim_cache_update_LRU(int );
                void print_victim();
                int read_count;
                int read_miss_count;
                int write_count;
                int write_miss_count;
                int mem_req_count;
                int write_back_count;
                int swap_request_count;
                int successful_swaps;
                void print_stat();
};
