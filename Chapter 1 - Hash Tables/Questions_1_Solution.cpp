#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace std;
using namespace chrono;

// Configuration
const int NUM_BUCKETS        = 16;
const int ENTRIES_PER_BUCKET = 4;
const int N = NUM_BUCKETS * ENTRIES_PER_BUCKET;   // capacity

// Hash function
int hash_key(const string& key) {
    int h = 0;
    for (char c : key)
        h = h * 31 + c;
    return abs(h) % NUM_BUCKETS;
}

// Bucket
struct Bucket {
    string   entries[ENTRIES_PER_BUCKET];
    int      count    = 0;
    Bucket*  overflow = nullptr;
};

// Hash Table
Bucket table[NUM_BUCKETS];

void insert(const string& key) {
    Bucket* b = &table[hash_key(key)];

    // walk the chain until there is a free slot
    while (b->count == ENTRIES_PER_BUCKET) {
        if (b->overflow == nullptr)
            b->overflow = new Bucket();
        b = b->overflow;
    }

    b->entries[b->count++] = key;
}

// Return the total number of buckets
int bucket_total(int i) {
    int total = 0;
    for (Bucket* b = &table[i]; b != nullptr; b = b->overflow)
        total += b->count;
    return total;
}

// Statistics
void print_stats() {
    vector<int> counts(NUM_BUCKETS);
    for (int i = 0; i < NUM_BUCKETS; i++)
        counts[i] = bucket_total(i);

    int    mn  = *min_element(counts.begin(), counts.end());
    int    mx  = *max_element(counts.begin(), counts.end());
    double avg = (double)accumulate(counts.begin(), counts.end(), 0) / NUM_BUCKETS;

    cout << "  Min entries/bucket: " << mn  << "\n";
    cout << "  Avg entries/bucket: " << avg << "\n";
    cout << "  Max entries/bucket: " << mx  << "\n";
}

// ASCII time graph
void print_graph(const vector<double>& times) {
    const int COLS = 60, ROWS = 10;

    vector<double> samples(COLS);
    int step = max(1, (int)times.size() / COLS);
    for (int c = 0; c < COLS; c++) {
        int start = c * step;
        int end   = min(start + step, (int)times.size());
        double sum = 0;
        for (int i = start; i < end; i++) sum += times[i];
        samples[c] = sum / (end - start);
    }

    double maxT = *max_element(samples.begin(), samples.end());

    for (int row = ROWS - 1; row >= 0; row--) {
        cout << "  ";
        for (int col = 0; col < COLS; col++) {
            int bar = (int)(samples[col] / maxT * (ROWS - 1));
            cout << (bar >= row ? '|' : ' ');
        }
        cout << "\n";
    }
    cout << "  " << string(COLS, '-') << "\n";
    cout << "  0" << string(COLS - 4, ' ') << times.size() << " items\n";
    cout << "  (peak = " << maxT << " us)\n";
}

// Reset table between experiments
void reset_table() {
    for (int i = 0; i < NUM_BUCKETS; i++) {
        Bucket* ov = table[i].overflow;
        while (ov) { Bucket* next = ov->overflow; delete ov; ov = next; }
        table[i].count    = 0;
        table[i].overflow = nullptr;
    }
}

// Run one experiment
void run(const string& label, int total) {
    reset_table();
    vector<double> times;
    times.reserve(total);

    for (int i = 0; i < total; i++) {
        string key = "key" + to_string(i);
        auto t0 = high_resolution_clock::now();
        insert(key);
        auto t1 = high_resolution_clock::now();
        times.push_back(duration<double, micro>(t1 - t0).count());
    }

    cout << "\n=== " << label << " (" << total << " inserts) ===\n";
    print_stats();
    cout << "Insertion time vs items inserted:\n";
    print_graph(times);
}

// Main
int main() {
    cout << "Hash Table: " << NUM_BUCKETS << " buckets x "
         << ENTRIES_PER_BUCKET << " entries = N=" << N << "\n";

    run("N",    N);
    run("10N",  10 * N);
    run("100N", 100 * N);
    run("1000N", 1000 * N);
}
