#ifndef __SEARCHRESULT_H
#define __SEARCHRESULT_H
#include "Common.h"
#include "GlobalAddress.h"

struct SearchResult {
    bool is_leaf;
    uint8_t level;
    GlobalAddress slibing;
    GlobalAddress next_level;
    Value val;
};
#endif
