#ifndef TOKEN_BUCKET_H
#define TOKEN_BUCKET_H

#include <chrono>

// TokenBucket
// OOP concept: Encapsulation — private state, controlled access via methods.
// Purpose: Rate limiting — each client gets a bucket that refills over time.
class TokenBucket {
private:
    double capacity;
    double tokens;
    double refillRatePerSec;
    std::chrono::steady_clock::time_point lastRefillTime;

    void refill() {
        auto now = std::chrono::steady_clock::now();
        double elapsedSeconds =
            std::chrono::duration<double>(now - lastRefillTime).count();

        double tokensToAdd = elapsedSeconds * refillRatePerSec;
        double newTokens = tokens + tokensToAdd;
        tokens = (newTokens > capacity) ? capacity : newTokens;
        lastRefillTime = now;
    }

public:
    TokenBucket() : capacity(0), tokens(0), refillRatePerSec(0),
                     lastRefillTime(std::chrono::steady_clock::now()) {}

    TokenBucket(double cap, double refillRate)
        : capacity(cap), tokens(cap), refillRatePerSec(refillRate),
          lastRefillTime(std::chrono::steady_clock::now()) {}

    bool allowRequest() {
        refill();
        if (tokens >= 1.0) {
            tokens -= 1.0;
            return true;
        }
        return false;
    }

    double availableTokens() {
        refill();
        return tokens;
    }
};

#endif
