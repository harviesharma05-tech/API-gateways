#include <iostream>
#include <vector>
#include <queue>
#include <string>

using namespace std;

struct Worker {
    string name;
    int queueSize;
    bool healthy;
};

// Select worker with the smallest queue (basic dynamic load balancing)
Worker selectWorker(const vector<Worker>& workers) {
    Worker best = workers[0];

    for (const auto& worker : workers) {
        if (worker.healthy && worker.queueSize < best.queueSize) {
            best = worker;
        }
    }

    return best;
}

int main() {

    vector<Worker> workers = {
        {"Worker-1", 5, true},
        {"Worker-2", 2, true},
        {"Worker-3", 8, true}
    };

    cout << "Dynamic Load Balancer\n";
    cout << "---------------------\n";

    Worker selected = selectWorker(workers);

    cout << "Selected Worker: " << selected.name << endl;
    cout << "Current Queue: " << selected.queueSize << endl;

    return 0;
}
