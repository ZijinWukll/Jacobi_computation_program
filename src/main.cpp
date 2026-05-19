#include "jacobi.h"
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <algorithm>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int k;
    cin >> k;
    cin.ignore(1, '\n');

    // Read all input first
    vector<string> ms(100), ns(100);
    for (int i = 0; i < 100; i++) {
        getline(cin, ms[i]);
        getline(cin, ns[i]);
    }

    // Compute Jacobi symbols in parallel
    vector<int> results(100);
    int num_threads = max(1u, thread::hardware_concurrency());
    vector<future<void>> futures;

    auto worker = [&](int start, int end) {
        for (int i = start; i < end; i++) {
            BigInt m(ms[i]), n(ns[i]);
            results[i] = jacobi(m, n);
        }
    };

    // Split 100 pairs evenly among threads
    int chunk = 100 / num_threads;
    int remainder = 100 % num_threads;
    int pos = 0;
    for (int t = 0; t < num_threads; t++) {
        int count = chunk + (t < remainder ? 1 : 0);
        if (count > 0) {
            futures.push_back(async(launch::async, worker, pos, pos + count));
            pos += count;
        }
    }
    for (auto& f : futures) f.get();

    // Output results in order
    for (int r : results) cout << r << '\n';
    return 0;
}
