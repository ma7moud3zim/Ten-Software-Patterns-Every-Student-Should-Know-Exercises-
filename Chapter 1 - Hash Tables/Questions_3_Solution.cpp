#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>

using namespace std;
using namespace chrono;

// Configuration
const int NUM_BUCKETS        = 16;
const int ENTRIES_PER_BUCKET = 4;
const int N = NUM_BUCKETS * ENTRIES_PER_BUCKET;   // 64

// Hash function
int hash_key(const string& key) {
    int h = 0;
    for (char c : key) h = h * 31 + c;
    return abs(h) % NUM_BUCKETS;
}

// Bucket
struct Bucket {
    string  entries[ENTRIES_PER_BUCKET];
    int     count    = 0;
    Bucket* overflow = nullptr;

    // remove entry at index i by swapping with the last entry
    void remove(int i) {
        entries[i] = entries[--count];
        entries[count] = "";
    }
};

// Hash Table
Bucket table[NUM_BUCKETS];
int    total_entries = 0;

void insert(const string& key) {
    Bucket* b = &table[hash_key(key)];
    while (b->count == ENTRIES_PER_BUCKET) {
        if (b->overflow == nullptr) b->overflow = new Bucket();
        b = b->overflow;
    }
    b->entries[b->count++] = key;
    total_entries++;
}

// Delete key — returns true if found and removed
bool delete_key(const string& key) {
    Bucket* b = &table[hash_key(key)];
    while (b != nullptr) {
        for (int i = 0; i < b->count; i++) {
            if (b->entries[i] == key) {
                b->remove(i);
                total_entries--;
                return true;
            }
        }
        b = b->overflow;
    }
    return false;
}

// Statistics
int bucket_total(int i) {
    int total = 0;
    for (Bucket* b = &table[i]; b != nullptr; b = b->overflow)
        total += b->count;
    return total;
}

void print_stats() {
    vector<int> counts(NUM_BUCKETS);
    for (int i = 0; i < NUM_BUCKETS; i++)
        counts[i] = bucket_total(i);

    int    mn  = *min_element(counts.begin(), counts.end());
    int    mx  = *max_element(counts.begin(), counts.end());
    double avg = (double)accumulate(counts.begin(), counts.end(), 0) / NUM_BUCKETS;

    cout << "  Total entries in table : " << total_entries << "\n";
    cout << "  Min entries/bucket     : " << mn  << "\n";
    cout << "  Avg entries/bucket     : " << avg << "\n";
    cout << "  Max entries/bucket     : " << mx  << "\n";
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
        for (int col = 0; col < COLS; col++)
            cout << ((int)(samples[col] / maxT * (ROWS-1)) >= row ? '|' : ' ');
        cout << "\n";
    }
    cout << "  " << string(COLS, '-') << "\n";
    cout << "  0" << string(COLS - 4, ' ') << times.size() << " items\n";
    cout << "  (peak = " << maxT << " us)\n";
}

// Reset table
void reset_table() {
    for (int i = 0; i < NUM_BUCKETS; i++) {
        Bucket* ov = table[i].overflow;
        while (ov) { Bucket* next = ov->overflow; delete ov; ov = next; }
        table[i].count    = 0;
        table[i].overflow = nullptr;
    }
    total_entries = 0;
}

// Main
int main() {
    cout << "N=" << N << "  (100N=" << 100*N << " initial inserts, "
         << "50N=" << 50*N << " deletes, 50N=" << 50*N << " new inserts)\n";

    // ── Step 1: insert 100N entries, record all keys ───────────
    cout << "\n[Step 1] Inserting 100N = " << 100*N << " entries...\n";
    vector<string> all_keys;
    all_keys.reserve(100 * N);
    for (int i = 0; i < 100 * N; i++) {
        string key = "key" + to_string(i);
        insert(key);
        all_keys.push_back(key);
    }
    cout << "  Done. Table has " << total_entries << " entries.\n";

    // ── Step 2: delete a random 50N keys ──────────────────────
    cout << "\n[Step 2] Deleting random 50N = " << 50*N << " keys...\n";
    mt19937 rng(42);
    shuffle(all_keys.begin(), all_keys.end(), rng);
    vector<string> to_delete(all_keys.begin(), all_keys.begin() + 50*N);
    vector<string> remaining(all_keys.begin() + 50*N, all_keys.end());

    for (auto& key : to_delete)
        delete_key(key);

    cout << "  Done. Table now has " << total_entries << " entries.\n";
    cout << "\n  Stats after deletion:\n";
    print_stats();

    // ── Step 3: insert 50N new keys, time each one ────────────
    cout << "\n[Step 3] Inserting 50N = " << 50*N << " new keys...\n";
    vector<double> times;
    times.reserve(50 * N);

    for (int i = 100*N; i < 150*N; i++) {
        string key = "newkey" + to_string(i);
        auto t0 = high_resolution_clock::now();
        insert(key);
        auto t1 = high_resolution_clock::now();
        times.push_back(duration<double, micro>(t1 - t0).count());
    }

    double avg_t = accumulate(times.begin(), times.end(), 0.0) / times.size();
    double min_t = *min_element(times.begin(), times.end());
    double max_t = *max_element(times.begin(), times.end());

    cout << "\n  Stats after re-insertion:\n";
    print_stats();
    cout << "\n  Timing (50N inserts into partially-empty table):\n";
    cout << "  Min insert : " << min_t << " us\n";
    cout << "  Avg insert : " << avg_t << " us\n";
    cout << "  Max insert : " << max_t << " us\n";
    cout << "\n  Insertion time vs items inserted:\n";
    print_graph(times);
}
