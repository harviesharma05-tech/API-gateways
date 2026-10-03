#ifndef TOKENBUCKET_H
#define TOKENBUCKET_H
#ifdef __cplusplus
extern "C" {
#endif
/* Token bucket rate limiter. Tokens refill continuously at `rate` per second up to `capacity`.
   Time (seconds) is passed in by the caller, which keeps the structure easy to test. */
typedef struct { double tokens, capacity, rate, last; } TokenBucket;
void tb_init(TokenBucket *tb, double capacity, double rate, double now);
int  tb_allow(TokenBucket *tb, double now);   /* 1 = request allowed, 0 = rejected */
#ifdef __cplusplus
}
#endif
#endif
