#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cstring>
#include <cstdint>

using namespace std;
using namespace chrono;

// Configuration
const int NUM_BUCKETS        = 16;
const int ENTRIES_PER_BUCKET = 4;
const int N_KEYS             = 3000;   // N > 2500

// Bucket / Table (Exercise 1)
struct Bucket {
    string  entries[ENTRIES_PER_BUCKET];
    int     count    = 0;
    Bucket* overflow = nullptr;
    bool full() const { return count == ENTRIES_PER_BUCKET; }
    void push(const string& k) { entries[count++] = k; }
};

struct HashTable {
    Bucket primary[NUM_BUCKETS];

    void insert(const string& key, int idx) {
        Bucket* b = &primary[idx % NUM_BUCKETS];
        while (b->full()) {
            if (!b->overflow) b->overflow = new Bucket();
            b = b->overflow;
        }
        b->push(key);
    }

    int chain_count(int i) const {
        int n = 0;
        for (const Bucket* b = &primary[i]; b; b = b->overflow) n += b->count;
        return n;
    }

    void clear() {
        for (int i = 0; i < NUM_BUCKETS; i++) {
            Bucket* ov = primary[i].overflow;
            while (ov) { Bucket* nx = ov->overflow; delete ov; ov = nx; }
            primary[i].count = 0; primary[i].overflow = nullptr;
        }
    }
};

// ══════════════════════════════════════════════════════════════
// Hash functions
// ══════════════════════════════════════════════════════════════

// 1. Knuth multiplicative hash
//    Folds the string into a 32-bit integer k, then computes
//    (k * A) >> (32 - log2(NUM_BUCKETS))
//    where A = 2654435761 = floor(2^32 / golden_ratio)
int knuth_hash(const string& key) {
    uint32_t k = 0;
    for (char c : key) k = k * 31 + c;
    const uint32_t A = 2654435761u;        // Knuth's constant
    return (k * A) >> (32 - 4);            // 4 = log2(16)
}

// 2. MurmurHash3 (32-bit)
int murmur_hash(const string& key) {
    const uint8_t* data = (const uint8_t*)key.data();
    int len = (int)key.size();
    uint32_t h = 0;
    const uint32_t c1 = 0xcc9e2d51, c2 = 0x1b873593;
    int nblocks = len / 4;
    for (int i = 0; i < nblocks; i++) {
        uint32_t k; memcpy(&k, data + i*4, 4);
        k *= c1; k = (k<<15)|(k>>17); k *= c2;
        h ^= k;  h = (h<<13)|(h>>19); h = h*5 + 0xe6546b64;
    }
    const uint8_t* tail = data + nblocks*4;
    uint32_t k = 0;
    switch (len & 3) {
        case 3: k ^= (uint32_t)tail[2] << 16; [[fallthrough]];
        case 2: k ^= (uint32_t)tail[1] << 8;  [[fallthrough]];
        case 1: k ^= tail[0];
                k *= c1; k = (k<<15)|(k>>17); k *= c2; h ^= k;
    }
    h ^= len;
    h ^= h>>16; h *= 0x85ebca6b; h ^= h>>13; h *= 0xc2b2ae35; h ^= h>>16;
    return (int)(h % NUM_BUCKETS);
}

// 3. SHA-256 (self-contained — no external library)
static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
    0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
    0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
    0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
    0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32-n)); }

static void sha256_block(uint32_t h[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i*4]<<24) | ((uint32_t)block[i*4+1]<<16)
             | ((uint32_t)block[i*4+2]<<8) | block[i*4+3];
    }
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i-15],7)^rotr32(w[i-15],18)^(w[i-15]>>3);
        uint32_t s1 = rotr32(w[i-2],17)^rotr32(w[i-2],19)^(w[i-2]>>10);
        w[i] = w[i-16]+s0+w[i-7]+s1;
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1  = rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
        uint32_t ch  = (e&f)^(~e&g);
        uint32_t tmp1= hh+S1+ch+K256[i]+w[i];
        uint32_t S0  = rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
        uint32_t maj = (a&b)^(a&c)^(b&c);
        uint32_t tmp2= S0+maj;
        hh=g; g=f; f=e; e=d+tmp1; d=c; c=b; b=a; a=tmp1+tmp2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
    h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
}

int sha256_hash(const string& key) {
    uint32_t h[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };
    // pad message
    vector<uint8_t> msg(key.begin(), key.end());
    uint64_t bitlen = msg.size() * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0);
    for (int i = 7; i >= 0; i--) msg.push_back((bitlen >> (i*8)) & 0xff);
    for (size_t i = 0; i < msg.size(); i += 64)
        sha256_block(h, msg.data() + i);
    return (int)(h[0] % NUM_BUCKETS);
}

// 4. Constant hash — worst case, everything in bucket 0
int constant_hash(const string& /*key*/) { return 0; }

// Experiment
struct Result {
    string name;
    int mn, mx; double avg;
    double total_ms, avg_us, max_us;
};

Result run(const string& name, int (*hash_fn)(const string&)) {
    HashTable ht;
    vector<double> times;
    times.reserve(N_KEYS);

    auto wall0 = high_resolution_clock::now();
    for (int i = 0; i < N_KEYS; i++) {
        string key = "key" + to_string(i);
        auto t0 = high_resolution_clock::now();
        ht.insert(key, hash_fn(key));
        auto t1 = high_resolution_clock::now();
        times.push_back(duration<double,micro>(t1-t0).count());
    }
    auto wall1 = high_resolution_clock::now();

    vector<int> counts(NUM_BUCKETS);
    for (int i = 0; i < NUM_BUCKETS; i++) counts[i] = ht.chain_count(i);

    Result r;
    r.name     = name;
    r.mn       = *min_element(counts.begin(), counts.end());
    r.mx       = *max_element(counts.begin(), counts.end());
    r.avg      = (double)accumulate(counts.begin(),counts.end(),0) / NUM_BUCKETS;
    r.total_ms = duration<double,milli>(wall1-wall0).count();
    r.avg_us   = accumulate(times.begin(),times.end(),0.0) / times.size();
    r.max_us   = *max_element(times.begin(),times.end());
    ht.clear();
    return r;
}

// Output
void print_table(const vector<Result>& res) {
    cout << "\n"
         << left  << setw(12) << "Hash"
         << right << setw(6)  << "Min"
                  << setw(7)  << "Avg"
                  << setw(7)  << "Max"
                  << setw(12) << "Total(ms)"
                  << setw(11) << "Avg(us)"
                  << setw(11) << "Max(us)" << "\n"
         << string(66,'-') << "\n";
    for (auto& r : res)
        cout << left  << setw(12) << r.name
             << right << setw(6)  << r.mn
                      << setw(7)  << fixed << setprecision(1) << r.avg
                      << setw(7)  << r.mx
                      << setw(12) << setprecision(3) << r.total_ms
                      << setw(11) << setprecision(3) << r.avg_us
                      << setw(11) << setprecision(3) << r.max_us << "\n";
    cout << string(66,'-') << "\n";
}

void print_bar(const vector<Result>& res) {
    int gmax = max_element(res.begin(),res.end(),
        [](auto& a,auto& b){return a.mx<b.mx;})->mx;
    cout << "\n  Max entries/bucket:\n";
    for (auto& r : res) {
        int bar = (int)((double)r.mx/gmax*40);
        cout << "  " << left << setw(10) << r.name << "|"
             << string(bar,'#') << " " << r.mx << "\n";
    }
    cout << "\n  Total insert time (ms):\n";
    double tmax = max_element(res.begin(),res.end(),
        [](auto& a,auto& b){return a.total_ms<b.total_ms;})->total_ms;
    for (auto& r : res) {
        int bar = (int)(r.total_ms/tmax*40);
        cout << "  " << left << setw(10) << r.name << "|"
             << string(bar,'#') << " " << fixed << setprecision(2) << r.total_ms << " ms\n";
    }
}

int main() {
    cout << "Exercise 4 — Hash Function Comparison\n";
    cout << "N=" << N_KEYS << " keys,  "
         << NUM_BUCKETS << " buckets x " << ENTRIES_PER_BUCKET << " slots\n";

    vector<Result> results = {
        run("Knuth",    knuth_hash),
        run("Murmur3",  murmur_hash),
        run("SHA-256",  sha256_hash),
        run("Constant", constant_hash),
    };

    print_table(results);
    print_bar(results);
}
