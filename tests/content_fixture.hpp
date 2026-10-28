#pragma once
#include "app.hpp"
#include <cstdio>
#include <cstdlib>

namespace spire_test {
// Actual-pack tests may load the complete private file in their host fixture;
// production hosts continue to use bounded range callbacks. MaxPack is only
// the historical 0.16 fixture limit and cannot truncate a world pack silently.
inline size_t pack_size(FILE* file){
    if(!file||std::fseek(file,0,SEEK_END))std::abort();
    const long size=std::ftell(file);
    if(size<40||static_cast<unsigned long>(size)>spire::MaxContentFile||std::fseek(file,0,SEEK_SET)){
        std::fprintf(stderr,"Invalid private test pack length: %ld\n",size);
        std::abort();
    }
    return static_cast<size_t>(size);
}
}
