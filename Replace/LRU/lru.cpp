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
class LRUCache
{
    DLinkedNode *head;
    DLinkedNode *tail;
    long long int capacity;
    unordered_map<long long int, DLinkedNode *> cache; // id->{id, value, prev, next}
    long long int cache_size = 0;
    long long int total_num = 0, hit_num = 0;

public:
    LRUCache(long long int _capacity) : capacity(_capacity)
    {
        head = new DLinkedNode();
        tail = new DLinkedNode();
        head->next = tail, tail->prev = head;
    }

    ~LRUCache()
    {
        for (pair<const long long int, DLinkedNode *> &block : cache)
        {
            delete block.second;
            block.second = nullptr;
        }
        delete head;
        head = nullptr;
        delete tail;
        tail = nullptr;
    }

    //供外部调用
    void visit(long long int key, long long int value)
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
    void set(long long int key, long long int value)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache[key] = node;
        addToHead(node);
        cache_size += value;
    }
    void evict(long long int key, long long int value)
    {
        while (cache_size + value > capacity)
        {
            DLinkedNode *removed = removeTail();
            cache_size -= removed->value;
            cache.erase(removed->key);
            delete removed;
            removed = nullptr;
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
int main(int argc, char *argv[])
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter LRUCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    LRUCache cache(capa);

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
    cout << "-------LRU替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------LRU替换算法-------" << endl;

    fin_test.close();
}