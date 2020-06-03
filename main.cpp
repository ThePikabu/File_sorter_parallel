#include <iostream>
#include <fstream>
#include "fileMaker.h"
#include <algorithm>
#include <thread>
#include <iosfwd>
#include <vector>
#include <queue>
#include <numeric>

using namespace std;



template<typename T>
class SafeBinaryInputStream {
    ifstream f;
    mutable mutex m;
    unsigned long int size_f;

public:
    SafeBinaryInputStream(string filename) {
        lock_guard<mutex> g(m);
        f.open(filename, ios::in | ios::binary);
        f.seekg(0, f.end);
        size_f = f.tellg();
        f.seekg(0, f.beg);

        cout << "File size = " << size_f / sizeof(int) << '\n';
    }

    ~SafeBinaryInputStream() {
        lock_guard<mutex> g(m);
        f.close();
    }

    T *read(int &num) {
        lock_guard<mutex> g(m);

        unsigned long int pos = f.tellg();

        T *res;
        if (f.eof() || pos == f.end)
            return nullptr;
        int new_num = min((unsigned long int) num, (size_f - pos) / sizeof(int));

        if(new_num<=0)
            return nullptr;

        res = new T[new_num];
        f.read((char *) res, new_num * sizeof(T));

        num = new_num;

        return res;


    }
};

class SafeQueue{
    queue<int> q;
    condition_variable c;
    mutable mutex m;

    mutable mutex vector_m;
    vector<bool> everybody_works;

public:
    SafeQueue(int thread_num){
        everybody_works.assign(thread_num, false);
    }

    SafeQueue(SafeQueue const& other){
        lock_guard<mutex> g(other.m);
        lock_guard<mutex> gv(other.vector_m);
        q = other.q;
        everybody_works = other.everybody_works;
    }

    SafeQueue(queue<int> const& default_queue, int thread_num){
        q = default_queue;
        everybody_works.assign(thread_num, false);
    }

    void push(int val){
        lock_guard<mutex> g(m);
        q.push(val);
        c.notify_one();
    }

    int size(){
        lock_guard<mutex> g(m);
        return q.size();
    }

    void set_me_working(int th, bool val){
        lock_guard<mutex> g(vector_m);
        everybody_works[th] = val;
        c.notify_one();
    }

    bool is_everybody_working(){
        lock_guard<mutex> g(vector_m);
        return accumulate(everybody_works.begin(), everybody_works.end(), false);
    }

    int just_pop(){
        lock_guard<mutex> lk(m);
        if(q.empty())
            throw "no elements!";
        int a = q.front();
        q.pop();
        return a;
    }

    bool wait_pop(int& a, int& b){
        unique_lock<mutex> lk(m);
        c.wait(lk, [this] {return q.size()>1 || !is_everybody_working();});
        if(q.empty()){
            throw "Bad behavior";
        }
        if(q.size() == 1)
            return false;
        a = q.front();
        q.pop();
        b = q.front();
        q.pop();
        return true;
    }
};

void thread_work(SafeQueue& q, int my_num){
    while(true) {
        int a, b;
        bool go = q.wait_pop(a, b);

        if (!go)
            break;

        q.set_me_working(my_num, true);
        int m = max(a, b);
        this_thread::sleep_for(chrono::milliseconds(100));
        q.set_me_working(my_num, false);
    }

    q.set_me_working(my_num, false);
}

void read_work(SafeBinaryInputStream<int> &f, int part_size, int thread_num) {
    int file_num = 0;
    string thread_s = to_string(thread_num);
    while (true) {
        int num = part_size;
        auto *part = f.read(num);
        //string thread_s = to_string(thread_num);

        if (part == nullptr) {
            //cout << "Блятб брейк на номере " << thread_num << '\n';
            break;
        }

        cout << "Numbers red: " << num << '\n';

        string file_s = to_string(file_num);

        fstream fout;
        fout.open(thread_s.append("_").append(file_s).append("_part").append(".bin"), ios::out | ios::binary);
        fout.write((char *) part, num * sizeof(int));
        fout.close();

        cout << "Numbers written: " << num << '\n';

        file_num++;
        delete[] part;
    }
}


int main() {
    int n = 10000;
//    cin >> n;
    fileMaker::makedata(n); // Making a main file

//    ifstream myFile;
//    myFile.open("data.bin", ios::in | ios::binary);
//
//
//    myFile.seekg(0, myFile.end);
//    unsigned long int size_f = myFile.tellg(); // size of main file
//    cout << size_f / sizeof(int) << '\n';
//    myFile.seekg(0, myFile.beg);
//
//
//    myFile.close();

    string filename = "data.bin";
    int block_size = n; // n = 10000

    block_size = block_size / 4;

    SafeBinaryInputStream<int> s(filename);

    vector<thread> ts(3);

    ts[0] = thread(read_work, ref(s), block_size, 0);
    ts[1] = thread(read_work, ref(s), block_size, 1);
    ts[2] = thread(read_work, ref(s), block_size, 2);

    read_work(s, block_size, 3);

    ts[0].join();
    ts[1].join();
    ts[2].join();



    return 0;
}