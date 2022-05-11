#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <queue>
#include <exception>
#include <random>
using namespace std;

#define AVERAGE_SIZE 77700
#define INTERVAL 45000
int MAX_YEWU_NUM;

struct DLinkedNode
{
    string key;
    long long int value;
    DLinkedNode *prev;
    DLinkedNode *next;
    DLinkedNode() : key("?"), value(0), prev(nullptr), next(nullptr) {}
    DLinkedNode(string _key, long long int _value) : key(_key), value(_value), prev(nullptr), next(nullptr) {}
};
class LRUCache
{
    DLinkedNode *head;
    DLinkedNode *tail;
    long long int capacity;
    unordered_map<string, DLinkedNode *> cache; // id->{id, value, prev, next}
    long long int cache_size = 0;

public:
    long long int total_num = 0, hit_num = 0;
    LRUCache(long long int capacity = 0)
    {
        this->capacity = capacity;
        head = new DLinkedNode();
        tail = new DLinkedNode();
        head->next = tail, tail->prev = head;
    }
    ~LRUCache()
    {
        for (pair<const string, DLinkedNode *> &block : cache)
            delete block.second;
        delete head;
        delete tail;
    }
    //供外部调用
    void visit(string key, long long int value)
    {
        total_num++;
        if (get(key))
        {
            hit_num++;
            moveToHead(cache[key]); // promotion
        }
        else
        {
            if (cache_size + value <= capacity)
            {
                set(key, value); // insertion
            }
            else
            {
                if (value > capacity)
                    return;
                evict(key, value); // evict
                set(key, value);   // insertion
            }
        }
    }
    void setCap(long long int _capacity) { capacity = _capacity; }

private:
    // get只判断 lru 缓存中是否有对象，不做 promotion 操作
    bool get(string key)
    {
        return cache.count(key);
    }
    void set(string key, long long int value)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache[key] = node;
        addToHead(node);
        cache_size += value;
    }
    void evict(string key, long long int value)
    {
        while (cache_size + value > capacity)
        {
            DLinkedNode *removed = removeTail();
            cache_size -= removed->value;
            cache.erase(removed->key);
            delete removed;
        }
    }
    void addToHead(DLinkedNode *node)
    {
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }

    void removeNode(DLinkedNode *node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(DLinkedNode *node)
    {
        removeNode(node);
        addToHead(node);
    }

    DLinkedNode *removeTail()
    {
        DLinkedNode *node = tail->prev;
        removeNode(node);
        return node;
    }
};

class PriSM
{
    long long int capacity, cache_size, interval_miss;
    vector<unordered_map<string, DLinkedNode *>> cache;
    vector<DLinkedNode *> head, tail;
    vector<long long int> yewu_size, yewu_miss, yewu_hit;
    vector<double> E;
    vector<LRUCache> shadow_cache;

public:
    PriSM(long long int capacity) : cache(MAX_YEWU_NUM), head(MAX_YEWU_NUM), tail(MAX_YEWU_NUM), yewu_size(MAX_YEWU_NUM), yewu_miss(MAX_YEWU_NUM), yewu_hit(MAX_YEWU_NUM), E(MAX_YEWU_NUM), shadow_cache(MAX_YEWU_NUM)
    {
        this->capacity = capacity;
        this->cache_size = this->interval_miss = 0;
        for (int i = 0; i < MAX_YEWU_NUM; i++)
        {
            cache[i].clear();
            head[i] = new DLinkedNode();
            tail[i] = new DLinkedNode();
            head[i]->next = tail[i], tail[i]->prev = head[i];
            yewu_size[i] = yewu_miss[i] = yewu_hit[i] = 0;
            E[i] = 1.0;
            shadow_cache[i].setCap(capacity);
        }
    }
    ~PriSM()
    {
        for (int i = 0; i < MAX_YEWU_NUM; i++)
        {
            for (pair<const string, DLinkedNode *> &block : cache[i])
            {
                delete block.second;
            }
            delete head[i];
            delete tail[i];
        }
    }

    void visit(string key, long long int value, int src)
    {
        shadow_cache[src].visit(key, value);
        if (cache[src].count(key))
        {
            yewu_hit[src]++;
            moveToHead(cache[src][key], src);
        }
        else
        {
            interval_miss++;
            yewu_miss[src]++;
            if (cache_size + value <= capacity)
            {
                set(key, value, src);
            }
            else
            {
                if (value > capacity)
                    return;
                evict(key, value);
                set(key, value, src);
            }

            if (interval_miss % INTERVAL == 0)
            {
                update();
            }
        }
    }

    bool get(string key, int src) const
    {
        return cache[src].count(key);
    }

private:
    void set(string key, long long int value, int src)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache[src][key] = node;
        yewu_size[src] += value;
        addToHead(node, src);
        cache_size += value;
    }

    //重点关注
    void evict(string key, long long int value)
    {
        std::random_device rd; // linux下 rd() 产生的是真随机数， windows下产生的是伪随机数
        std::mt19937 gen(rd());
        std::discrete_distribution<> d(begin(E), end(E));
        while (cache_size + value > capacity)
        {
            int victim_id = d(gen);
            if (yewu_size[victim_id] == 0) //碰巧该业务在缓存中没有项目时，顺序选一个直接淘汰
            {
                for (int i = 0; i < MAX_YEWU_NUM; i++)
                {
                    if (yewu_size[i] > 0)
                    {
                        DLinkedNode *removed = removeTail(i);
                        cache_size -= removed->value;
                        yewu_size[i] -= removed->value;
                        cache[i].erase(removed->key);
                        delete removed;
                    }
                }
            }
            else
            {
                DLinkedNode *removed = removeTail(victim_id);
                cache_size -= removed->value;
                yewu_size[victim_id] -= removed->value;
                cache[victim_id].erase(removed->key);
                delete removed;
            }
        }
    }

    //更新参数
    void update()
    {
        long long int n = capacity / AVERAGE_SIZE, total_gain = 0;
        double c[MAX_YEWU_NUM], m[MAX_YEWU_NUM], t[MAX_YEWU_NUM], p[MAX_YEWU_NUM];
        for (int i = 0; i < MAX_YEWU_NUM; i++)
        {
            c[i] = (double)yewu_size[i] / cache_size;
            m[i] = (double)yewu_miss[i] / interval_miss;
            p[i] = shadow_cache[i].hit_num - yewu_hit[i];
            total_gain += p[i];
        }
        double all = 0.0;
        for (int i = 0; i < MAX_YEWU_NUM; i++)
        {
            t[i] = c[i] * (1 + p[i] / total_gain);
            all += t[i];
        }
        for (int i = 0; i < MAX_YEWU_NUM; i++)
        {
            t[i] /= all;
            double temp = m[i] + (c[i] - t[i]) * n / INTERVAL;
            if (total_gain == 0)
                E[i] = m[i];
            else if (temp < 0)
                E[i] = 0;
            else if (temp > 1)
                E[i] = 1;
            else
                E[i] = temp;
        }
        init();
    }

    void init()
    {
        interval_miss = 0;
        for (int i = 0; i < MAX_YEWU_NUM; i++)
        {
            yewu_hit[i] = yewu_miss[i] = shadow_cache[i].hit_num = shadow_cache[i].total_num = 0;
        }
    }

    void addToHead(DLinkedNode *node, int target)
    {
        node->prev = head[target];
        node->next = head[target]->next;
        head[target]->next->prev = node;
        head[target]->next = node;
    }

    void removeNode(DLinkedNode *node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(DLinkedNode *node, int target)
    {
        removeNode(node);
        addToHead(node, target);
    }

    DLinkedNode *removeTail(int src)
    {
        DLinkedNode *node = tail[src]->prev;
        removeNode(node);
        return node;
    }
};

int main()
{
    const long long int GB = 1024 * 1024 * 1024;
    ofstream fout("./mrc_prism.txt", ios::app);
    string line, temp;
    stringstream splitline;
    double rates[] = {
        0.001};
    long long int wss = (long long int)2 * 1024 * GB;
    fout << "预热40000000" << endl;

    for (MAX_YEWU_NUM = 64; MAX_YEWU_NUM <= 512; MAX_YEWU_NUM *= 2)
    {
        fout << "更新间隔:" << INTERVAL << endl;
        for (double rate : rates)
        {
            PriSM cache(wss * rate);
            fstream fin_warm("./warm.txt");
            fstream fin_test("./test.txt");

            long long int total_num = 0, total_miss = 0;
            while (getline(fin_warm, line)) //预热
            {
                splitline.clear();
                splitline.str(line);
                vector<string> data;
                while (getline(splitline, temp, ' '))
                {
                    data.emplace_back(temp);
                }
                int id = stol(data[2]) >= MAX_YEWU_NUM ? MAX_YEWU_NUM - 1 : stol(data[2]);
                cache.visit(data[1], stoll(data[0]), id);
            }
            fin_warm.close();

            while (getline(fin_test, line))
            {
                splitline.clear();
                splitline.str(line);
                vector<string> data;
                while (getline(splitline, temp, ' '))
                {
                    data.emplace_back(temp);
                }
                int id = stol(data[2]) >= MAX_YEWU_NUM ? MAX_YEWU_NUM - 1 : stol(data[2]);
                if (!cache.get(data[1], id))
                    total_miss++;
                cache.visit(data[1], stoll(data[0]), id);
                total_num++;
            }
            fout << rate * 100 << "% " << total_num - total_miss << " " << (double)(total_num - total_miss) / total_num << endl;
            fin_test.close();
        }
    }
    fout.close();
}