#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
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
class S4LRUCache
{
public:
    DLinkedNode *head[3];
    DLinkedNode *tail[3];
    long long int capacity;
    unordered_map<long long int, DLinkedNode *> cache_stack[3]; //id->{key, value, prev, next}
    long long int cache_size[3], hit_num, total_num;
    S4LRUCache(long long int capacity)
    {
        this->capacity = capacity;
        cache_size[0] = cache_size[1] = cache_size[2] = 0;
        hit_num = total_num = 0;
        head[0] = new DLinkedNode(), head[1] = new DLinkedNode(), head[2] = new DLinkedNode();
        tail[0] = new DLinkedNode(), tail[1] = new DLinkedNode(), tail[2] = new DLinkedNode();
        head[0]->next = tail[0], tail[0]->prev = head[0];
        head[1]->next = tail[1], tail[1]->prev = head[1];
        head[2]->next = tail[2], tail[2]->prev = head[2];
    }
    bool get(long long int key)
    {
        if (cache_stack[0].count(key))
        {
            moveToHead(cache_stack[0][key], 0);
            return true;
        }
        else if (cache_stack[1].count(key))
        {
            cache_size[0] += cache_stack[1][key]->value;
            moveToHead(cache_stack[1][key], 0);
            cache_stack[0][key] = cache_stack[1][key];
            cache_size[1] -= cache_stack[1][key]->value;
            cache_stack[1].erase(key);
        }
        else if (cache_stack[2].count(key))
        {
            cache_size[1] += cache_stack[2][key]->value;
            moveToHead(cache_stack[2][key], 1);
            cache_stack[1][key] = cache_stack[2][key];
            cache_size[2] -= cache_stack[2][key]->value;
            cache_stack[2].erase(key);
        }
        else
        {
            return false;
        }
        balance();
        return true;
    }
    void set(long long int key, long long int value)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache_stack[2][key] = node;
        addToHead(node, 2);
        cache_size[2] += value;
    }
    void evict(long long int key, long long int value)
    {
        while (cache_size[0] + cache_size[1] + cache_size[2] + value > capacity)
        {
            DLinkedNode *removed = removeTail(2);
            cache_size[2] -= removed->value;
            cache_stack[2].erase(removed->key);
            delete removed;
        }
    }
    void visit(long long int key, long long int value)
    {
        total_num++;
        if (get(key)) //get里面做了 premotion
        {
            hit_num++;
        }
        else
        {
            if (cache_size[0] + cache_size[1] + cache_size[2] + value <= capacity)
                set(key, value);
            else
            {
                evict(key, value);
                set(key, value);
            }
        }
    }
    // 命中后进行完 promotion 后，可能导致高级链容量超出，则将高级链的LRU端块移动到较低级链中，直到容量合适
    void balance()
    {
        while (cache_size[0] * 3 > capacity)
        {
            DLinkedNode *removed = removeTail(0);
            cache_size[1] += cache_stack[0][removed->key]->value;
            cache_stack[1][removed->key] = removed;
            cache_size[0] -= cache_stack[0][removed->key]->value;
            cache_stack[0].erase(removed->key);
            addToHead(removed, 1);
        }
        while (cache_size[1] * 3 > capacity)
        {
            DLinkedNode *removed = removeTail(1);
            cache_size[2] += cache_stack[1][removed->key]->value;
            cache_stack[2][removed->key] = removed;
            cache_size[1] -= cache_stack[1][removed->key]->value;
            cache_stack[1].erase(removed->key);
            addToHead(removed, 2);
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
    ~S4LRUCache()
    {
        for (int i = 0; i <= 2; i++)
        {
            for (pair<long long int, DLinkedNode *> block : cache_stack[i])
                delete block.second;
            delete head[i];
            delete tail[i];
        }
    }
};

int main()
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter S4LRUCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    S4LRUCache cache(capa);

    while (getline(fin_test, line))
    {
        splitline.clear();
        splitline.str(line);
        vector<string> data;
        while (getline(splitline, temp, ' '))
        {
            data.push_back(temp);
        }
        cache.visit(stoll(data[1]), stoll(data[0]));
    }
    cout << "-------S4LRU替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.total_num << ", 命中次数: " << cache.hit_num << endl;
    cout << "-------S4LRU替换算法-------" << endl;
}
