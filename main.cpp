/*
Copyright (c) 2015-2018 Justin Funston

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <string>

#include "system.h"

using namespace std;

int main()
{
   // tid_map is used to inform the simulator how
   // thread ids map to NUMA/cache domains. Using
   // the tid as an index gives the NUMA domain.
   unsigned int arr_map[] = {0, 1};
   vector<unsigned int> tid_map(arr_map, arr_map + 
         sizeof(arr_map) / sizeof(unsigned int));
   std::unique_ptr<SeqPrefetch> prefetch = std::make_unique<SeqPrefetch>();
   // The constructor parameters are:
   // the tid_map, the cache line size in bytes,
   // number of cache lines, the associativity,
   // the prefetcher object,
   // whether to count compulsory misses,
   // whether to do virtual to physical translation,
   // and number of caches/domains
   // WARNING: counting compulsory misses doubles execution time
   MultiCacheSystem sys(tid_map, 64, 4096, 8, std::move(prefetch), false, true, 1);
   char rw;
   string line;
   string header, load_store, state;
   string mem_access = "guest_mem_access_tlb";
   string user_mode = "0x3";
   uint64_t vaddr, paddr;
   uint64_t address;
   unsigned long long lines = 0;
   unsigned long long user_access = 0;
   ifstream infile;
   // This code works with the output from the 
   // ManualExamples/pinatrace pin tool
   // infile.open("pinatrace.out", ifstream::in);
   infile.open("../qemu-trace/nokvm-2020-12-01_-_03-59.log", ifstream::in);
   assert(infile.is_open());

   while(std::getline(infile, line))
   {
      infile >> header;
      size_t find = header.find(mem_access);
      //std::cout << lines << std::endl;
      //std::cout << header << header.length() << lines << std::endl;
      //printf("%s %d\n", line, lines);
      if(find != std::string::npos){
         // load or store
         infile >> load_store;
         AccessType accessType;
         //std::cout << load_store.at(load_store.length() - 2) << std::endl;
         if (load_store.at(load_store.length() - 2) == '0') {
            accessType = AccessType::Read;
         } else {
            accessType = AccessType::Write;
         }

         // state
         infile >> state;
         find = state.find(user_mode);
         if(find != std::string::npos){
             user_access++;
         }
         //std::cout << state << std::endl;

         // virtual address, physical address
         infile >> hex >> vaddr;
         infile >> hex >> paddr;

         if(vaddr != 0) {
            // By default the pinatrace tool doesn't record the tid,
            // so we make up a tid to stress the MultiCache functionality
            // sys.memAccess(address, accessType, lines%2);
            sys.memAccess(vaddr, accessType, 0);
         }
      } else {
         // setup tlb
         continue;
      }


      ++lines;
      if((lines % 1000000) == 1){
          cout<<lines<<endl;
      }
   }

   cout << "Accesses: " << lines << endl;
   cout << "Hits: " << sys.stats.hits << endl;
   cout << "Misses: " << lines - sys.stats.hits << endl;
   cout << "Hit rate: " << 1.0 * sys.stats.hits / lines << endl;
   cout << "User accesses: " << user_access << endl;
   cout << "User rate: " << 1.0 * user_access / lines << endl;
   cout << "Local reads: " << sys.stats.local_reads << endl;
   cout << "Local writes: " << sys.stats.local_writes << endl;
   cout << "Remote reads: " << sys.stats.remote_reads << endl;
   cout << "Remote writes: " << sys.stats.remote_writes << endl;
   cout << "Other-cache reads: " << sys.stats.othercache_reads << endl;
   //cout << "Compulsory Misses: " << sys.stats.compulsory << endl;
   
   infile.close();

   return 0;
}
