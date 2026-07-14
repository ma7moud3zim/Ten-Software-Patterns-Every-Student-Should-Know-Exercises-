#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
// The key difference before writing a single line: an extensible hash table
// rehashes and doubles when it gets too full, instead of chaining overflow
// buckets forever. That's the entire point of the exercise, comparing the
// two strategies.

using namespace std;
using namespace chrono;

// Configuration
const int INITIAL_BUCKETS    = 16;
const int ENTRIES_PER_BUCKET = 4;
const int N = INITIAL_BUCKETS * ENTRIES_PER_BUCKET;  // starting capacity
const double LOAD_FACTOR     = 0.75;  // rehash when 75% full

// Hash function
int hash_key(const string& key, int num_buckets) {
    int h = 0;
    for (char c : key) h = h * 31 + c;
    return abs(h) % num_buckets;
}

// Extensible Hash Table
struct HashTable {
    vector<vector<string>> buckets;
    int total = 0;

    HashTable(int size) : buckets(size) {}

    void insert(const string& key) {
        // rehash if load factor exceeded
        if ((double)total / buckets.size() >= LOAD_FACTOR)
            rehash();

        buckets[hash_key(key, buckets.size())].push_back(key);
        total++;
    }

    void rehash() {
        vector<vector<string>> old = move(buckets);
        buckets.assign(old.size() * 2, {});
        total = 0;
        for (auto& bucket : old)
            for (auto& key : bucket)
                insert(key);
    }
};

// Statistics
void print_stats(const HashTable& ht) {
    vector<int> counts;
    for (auto& b : ht.buckets) counts.push_back(b.size());

    int    mn  = *min_element(counts.begin(), counts.end());
    int    mx  = *max_element(counts.begin(), counts.end());
    double avg = (double)accumulate(counts.begin(), counts.end(), 0) / counts.size();

    cout << "  Min entries/bucket: " << mn  << "\n";
    cout << "  Avg entries/bucket: " << avg << "\n";
    cout << "  Max entries/bucket: " << mx  << "\n";
    cout << "  Final bucket count: " << ht.buckets.size() << "\n";
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

// Run one experiment
void run(const string& label, int total) {
    HashTable ht(INITIAL_BUCKETS);
    vector<double> times;
    times.reserve(total);

    for (int i = 0; i < total; i++) {
        string key = "key" + to_string(i);
        auto t0 = high_resolution_clock::now();
        ht.insert(key);
        auto t1 = high_resolution_clock::now();
        times.push_back(duration<double, micro>(t1 - t0).count());
    }

    cout << "\n=== " << label << " (" << total << " inserts) ===\n";
    print_stats(ht);
    cout << "\n  Insertion time vs items inserted:\n";
    print_graph(times);
}

// Main
int main() {
    cout << "Extensible Hash Table: starts at " << INITIAL_BUCKETS
         << " buckets x " << ENTRIES_PER_BUCKET << " = N=" << N
         << "  (rehash at load=" << LOAD_FACTOR << ")\n";

    run("N",    N);
    run("10N",  10 * N);
    run("100N", 100 * N);
    run("1000N", 1000 * N);
    run("100000N", 100000 * N);

}
