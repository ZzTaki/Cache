#include <cstdio>
#include <cstring>
#include <string>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
using namespace std;

#define INTERVAL 45000

int MAX_NUM, YEWU_NUM;

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
    void visit(string key, long long int value)
    {
        if (get(key))
        {
            moveToHead(cache[key]);
        }
        else
        {
            if (cache_size + value <= capacity)
            {
                set(key, value);
            }
            else
            {
                if (value > capacity)
                    return;
                evict(key, value);
                set(key, value);
            }
        }
    }
    bool get(string key) const
    {
        return cache.count(key);
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
    void setCap(long long int _capacity) { capacity = _capacity; }

private:
    void set(string key, long long int value)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache[key] = node;
        addToHead(node);
        cache_size += value;
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
class StackLRUCache
{
    vector<DLinkedNode *> head;
    vector<DLinkedNode *> tail;
    long long int capacity;
    vector<unordered_map<string, DLinkedNode *>> cache_stack; // id->{key, value, prev, next} 下标越小越靠近MRU端
    unordered_map<string, int> id_idx;                        // id->idx 对应id所在lru链段位置
    vector<long long int> cache_size;
    vector<long long int> cnt; //命中LRU链不同区间的数量

public:
    StackLRUCache(long long int capacity = 0) : head(MAX_NUM), tail(MAX_NUM), cache_stack(MAX_NUM), cache_size(MAX_NUM), cnt(MAX_NUM)
    {
        this->capacity = capacity;
        for (int i = 0; i < MAX_NUM; i++)
        {
            head[i] = new DLinkedNode();
            tail[i] = new DLinkedNode();
            head[i]->next = tail[i];
            tail[i]->prev = head[i];
            cache_size[i] = 0;
            cnt[i] = 0;
        }
    }
    ~StackLRUCache()
    {
        for (int i = 0; i < MAX_NUM; i++)
        {
            for (pair<string, DLinkedNode *> block : cache_stack[i])
                delete block.second;
            delete head[i];
            delete tail[i];
        }
    }
    void visit(string key, long long int value)
    {
        if (get(key))
        {
            int i = id_idx[key];
            moveToHead(cache_stack[i][key], 0); //将命中的文件放置在MRU端

            if (i != 0) //如果命中文件之前不在第一段，更新 cache_size 和 cache_stack
            {
                cache_size[0] += cache_stack[i][key]->value;
                cache_stack[0][key] = cache_stack[i][key];
                cache_size[i] -= cache_stack[i][key]->value;
                cache_stack[i].erase(key);
                balance();
                id_idx[key] = 0;
            }

            cnt[i]++;
        }
        else
        {
            if (value > capacity / MAX_NUM)
                return;
            set(key, value);
        }
    }
    void setCap(long long int _capacity) { capacity = _capacity; }
    vector<long long int> &getCnt() { return cnt; }
    void resetInterval()
    {
        for (int i = 0; i < MAX_NUM; i++)
            cnt[i] = 0;
    }

private:
    bool get(string key) const
    {
        return id_idx.count(key);
    }
    void set(string key, long long int value)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache_stack[0][key] = node;
        cache_size[0] += value;
        addToHead(node, 0);
        balance();
        id_idx[key] = 0;
    }

    bool balance() // lru发生evict，则返回true；否则返回false （仅用来判断是否预热结束；现已固定预热的长度，返回值不重要）
    {
        for (int j = 0; j < MAX_NUM - 1; j++)
        {
            if (cache_size[j] <= capacity / MAX_NUM)
            {
                return false;
            }
            while (cache_size[j] > capacity / MAX_NUM)
            {
                DLinkedNode *removed = removeTail(j);
                cache_size[j + 1] += cache_stack[j][removed->key]->value;
                cache_stack[j + 1][removed->key] = removed;
                cache_size[j] -= cache_stack[j][removed->key]->value;
                cache_stack[j].erase(removed->key);
                addToHead(removed, j + 1);
                id_idx[removed->key] = j + 1;
            }
        }
        if (cache_size[MAX_NUM - 1] <= capacity / MAX_NUM)
            return false;
        while (cache_size[MAX_NUM - 1] > capacity / MAX_NUM)
        {
            DLinkedNode *removed = removeTail(MAX_NUM - 1);
            cache_size[MAX_NUM - 1] -= removed->value;
            id_idx.erase(removed->key);
            cache_stack[MAX_NUM - 1].erase(removed->key);
            delete removed;
        }
        return true;
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
vector<int> greedyPart(vector<vector<long long int>> &cnts)
{
    vector<int> allocations(YEWU_NUM); //已分配给各业务的ways数量
    int balance = MAX_NUM - YEWU_NUM;  //剩余待分配的ways数量
    for (int i = 0; i < YEWU_NUM; i++)
    {
        allocations[i] = 1;
    }
    while (balance)
    {
        int winner = -1;
        long long int max_change = -1;
        for (int i = 0; i < YEWU_NUM; i++)
        {
            if (cnts[i][allocations[i]] > max_change)
            {
                max_change = cnts[i][allocations[i]];
                winner = i;
            }
        }
        allocations[winner]++;
        balance--;
    }
    return allocations;
}
int main()
{
    for (MAX_NUM = 32; MAX_NUM <= 2048 * 8; MAX_NUM *= 2)
    {
        for (YEWU_NUM = 2; YEWU_NUM <= 128 * 4; YEWU_NUM *= 2)
        {
            ofstream fout;
            const long long int GB = 1024 * 1024 * 1024;
            long long int capa = (long long int)2.2 * 1024 * GB * 0.01;

            // int update_nums = 0;
            // long double total_update_times = 0.0;
            fout.open("./mrc_ucp.txt", ios::app);
            fout << "更新间隔：" << INTERVAL << "，缓存容量：" << capa << "，业务数量：" << YEWU_NUM << "，分区数量：" << MAX_NUM << endl;
            long long int total_miss = 0, interval_miss = 0, total_num = 0;
            long long int one_capa = capa / MAX_NUM;
            vector<LRUCache> lru(YEWU_NUM);
            vector<StackLRUCache> StackLRU(YEWU_NUM);

            for (int i = 0; i < YEWU_NUM; i++)
            {
                lru[i].setCap(capa / YEWU_NUM);
                StackLRU[i].setCap(capa);
                StackLRU[i].resetInterval();
            }

            clock_t start = clock();
            ifstream fin("/data/home/tencent/partition/bj/118_yewu/warm.txt");
            while (true)
            {
                string key;
                long long int value;
                int id, temp;
                if (!(fin >> value >> key >> temp))
                    break;
                id = temp >= YEWU_NUM - 1 ? YEWU_NUM - 1 : temp;

                if (!lru[id].get(key))
                    interval_miss++;
                lru[id].visit(key, value);
                StackLRU[id].visit(key, value);
                if (interval_miss == INTERVAL)
                {
                    // timespec t1, t2;
                    // clock_gettime(CLOCK_MONOTONIC, &t1);
                    vector<vector<long long int>> cnts;
                    for (int i = 0; i < YEWU_NUM; i++)
                        cnts.push_back(StackLRU[i].getCnt());
                    vector<int> allocations = greedyPart(cnts);
                    for (int i = 0; i < YEWU_NUM; i++)
                    {
                        lru[i].setCap(allocations[i] * one_capa);
                        lru[i].evict("?", 0); //如果某一业务的缓存容量减少了，则驱逐那些lru端的多余文件
                        StackLRU[i].resetInterval();
                    }
                    // clock_gettime(CLOCK_MONOTONIC, &t2);
                    //预热过程中不计入总缺失中
                    interval_miss = 0;
                    // update_nums++;
                    // total_update_times += (long long int)(t2.tv_sec - t1.tv_sec) * 1000000000 + t2.tv_nsec - t1.tv_nsec;
                    // cout << total_update_times << endl;
                }
            }
            fin.close();
            interval_miss = 0;

            fin.open("/data/home/tencent/partition/bj/118_yewu/test.txt");
            while (true)
            {
                string key;
                long long int value;
                int id, temp;
                if (!(fin >> value >> key >> temp))
                    break;
                id = temp >= YEWU_NUM - 1 ? YEWU_NUM - 1 : temp;

                if (!lru[id].get(key))
                    interval_miss++;
                lru[id].visit(key, value);
                StackLRU[id].visit(key, value);
                if (interval_miss == INTERVAL)
                {
                    vector<vector<long long int>> cnts;
                    for (int i = 0; i < YEWU_NUM; i++)
                        cnts.push_back(StackLRU[i].getCnt());
                    vector<int> allocations = greedyPart(cnts);
                    for (int i = 0; i < YEWU_NUM; i++)
                    {
                        lru[i].setCap(allocations[i] * one_capa);
                        lru[i].evict("?", 0); //如果某一业务的缓存容量减少了，则驱逐那些lru端的多余文件
                        StackLRU[i].resetInterval();
                    }
                    total_miss += interval_miss;
                    interval_miss = 0;
                }
                total_num++;
            }
            fin.close();

            total_miss += interval_miss;
            clock_t end = clock();
            fout << "总用时：" << (double)(end - start) / (CLOCKS_PER_SEC * 60) << "分钟" << endl;
            fout << "总缺失：" << total_miss << "，命中率：" << (double)(total_num - total_miss) / total_num << endl;
            // fout << "预热阶段更新了" << update_nums << "次，"
            //      << "更新用时" << total_update_times << "纳秒" << endl;
            fout.close();
        }
    }
}