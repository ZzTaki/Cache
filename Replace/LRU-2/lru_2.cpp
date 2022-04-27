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

struct DLinkedNode
{
    long long int key, value;
    DLinkedNode *prev;
    DLinkedNode *next;
    DLinkedNode() : key(0), value(0), prev(nullptr), next(nullptr) {}
    DLinkedNode(long long int _key, long long int _value) : key(_key), value(_value), prev(nullptr), next(nullptr) {}
};
class LRU2Cache
{
    DLinkedNode *head[2];
    DLinkedNode *tail[2];
    long long int capacity;
    unordered_map<long long int, DLinkedNode *> cache;   // id->{id, value, prev, next}
    unordered_map<long long int, DLinkedNode *> history; //
    long long int cache_size = 0, history_size = 0;
    long long int total_num = 0, hit_num = 0;
    int para = 2; // history容量是 cache 容量的倍数

public:
    LRU2Cache(long long int _capacity) : capacity(_capacity)
    {
        head[0] = new DLinkedNode(), head[1] = new DLinkedNode();
        tail[0] = new DLinkedNode(), tail[1] = new DLinkedNode();
        head[0]->next = tail[0], tail[0]->prev = head[0];
        head[1]->next = tail[1], tail[1]->prev = head[1];
    }

    ~LRU2Cache()
    {
        for (pair<const long long int, DLinkedNode *> &block : cache)
        {
            delete block.second;
        }
        for (int i = 0; i < 2; i++)
        {
            delete head[i];
            delete tail[i];
        }
    }

    //供外部调用
    void visit(long long int key, long long int value)
    {
        total_num++;
        if (get(key))
        {
            hit_num++;
            moveToHead(cache[key], 0); // promotion
        }
        else
        {
            if (history.count(key)) //历史表中有记录
            {
                if (cache_size + value <= capacity)
                {
                    set(key, value, 0); // insertion
                }
                else
                {
                    evict(key, value, 0); // evict
                    set(key, value, 0);   // insertion
                }
            }
            else
            {
                if (history_size + value <= capacity * para)
                {
                    set(key, value, 1);
                }
                else
                {
                    if (value > capacity)
                        return;
                    evict(key, value, 1);
                    set(key, value, 1);
                }
            }
        }
    }

    long long int getTotal() const
    {
        return total_num;
    }

    long long int getHit() const
    {
        return hit_num;
    }

private:
    // get只判断 lru 缓存中是否有对象，不做 promotion 操作
    bool get(long long int key) const
    {
        return cache.count(key);
    }
    void set(long long int key, long long int value, int idx)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        if (idx) //加入history
        {
            history[key] = node;
            history_size += value;
        }
        else
        {
            cache[key] = node;
            cache_size += value;
        }
        addToHead(node, idx);
    }
    void evict(long long int key, long long int value, int idx)
    {
        if (idx)
        {
            while (history_size + value > para * capacity)
            {
                DLinkedNode *removed = removeTail(idx);
                history_size -= removed->value;
                history.erase(removed->key);
                delete removed;
                removed = nullptr;
            }
        }
        else
        {
            while (cache_size + value > capacity)
            {
                DLinkedNode *removed = removeTail(idx);
                cache_size -= removed->value;
                cache.erase(removed->key);
                delete removed;
                removed = nullptr;
            }
        }
    }

    void addToHead(DLinkedNode *node, int idx)
    {
        node->prev = head[idx];
        node->next = head[idx]->next;
        head[idx]->next->prev = node;
        head[idx]->next = node;
    }

    void removeNode(DLinkedNode *node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(DLinkedNode *node, int idx)
    {
        removeNode(node);
        addToHead(node, idx);
    }

    DLinkedNode *removeTail(int idx)
    {
        DLinkedNode *node = tail[idx]->prev;
        removeNode(node);
        return node;
    }
};
int main(int argc, char *argv[])
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter LRU-2Cache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    LRU2Cache cache(capa);

    while (getline(fin_test, line)) //跟踪
    {
        splitline.clear();
        splitline.str(line);
        vector<string> data;
        while (getline(splitline, temp, ' '))
        {
            data.push_back(temp);
        }
        //检查是否有某个块比缓存容量还大
        long long int key = stoll(data[1]), value = stoll(data[0]);

        cache.visit(key, value);
    }
    cout << "-------LRU-2替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------LRU-2替换算法-------" << endl;

    fin_test.close();
}