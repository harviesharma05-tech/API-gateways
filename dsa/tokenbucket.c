#include "tokenbucket.h"
void tb_init(TokenBucket *tb, double capacity, double rate, double now) {
    tb->tokens = capacity; tb->capacity = capacity; tb->rate = rate; tb->last = now;
}
int tb_allow(TokenBucket *tb, double now) {
    double add = (now - tb->last) * tb->rate;
    tb->tokens = tb->tokens + add > tb->capacity ? tb->capacity : tb->tokens + add;
    tb->last = now;
    if (tb->tokens >= 1.0) { tb->tokens -= 1.0; return 1; }
    return 0;
}
