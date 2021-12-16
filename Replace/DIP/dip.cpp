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
#include <time.h>
using namespace std;

struct DLinkedNode
{
    long long int key, value;
    DLinkedNode *prev;
    DLinkedNode *next;
    DLinkedNode() : key(0), value(0), prev(nullptr), next(nullptr) {}
    DLinkedNode(long long int _key, long long int _value) : key(_key), value(_value), prev(nullptr), next(nullptr) {}
};

//使用一个16位计数器来动态选择 LRU 和 BIP 策略
class DIPCache
{
public:
    DLinkedNode *head[3];
    DLinkedNode *tail[3];
    long long int capacity[3];
    unsigned short psel = 0;                              //police selector
    unordered_map<long long int, DLinkedNode *> cache[3]; //id->{id, value, prev, next}，0为真实缓存，1为 lru shadow 缓存，2为 bip shadow 缓存
    long long int cache_size[3];
    long long int total_num = 0, hit_num = 0;
    bool flag = false; //标记是否预热结束
    DIPCache(long long int capacity)
    {
        for (int i = 0; i < 3; i++)
        {
            head[i] = new DLinkedNode(), tail[i] = new DLinkedNode();
            head[i]->next = tail[i], tail[i]->prev = head[i];
            this->capacity[i] = capacity;
            cache[i].clear();
            cache_size[i] = 0;
        }
    }
    //get只判断缓存中是否有对象，不做 promotion 操作
    bool get(long long int key, int idx)
    {
        return cache[idx].count(key);
    }
    void set(long long int key, long long int value, int idx)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        cache[idx][key] = node;
        if (flag && (idx == 2 || (idx == 0 && psel > INT16_MAX))) //只有在预热结束 且 idx指示的是BIP缓存或者指示采用BIP策略的真实缓存 时才会插入tail位置
            addToTail(node, idx);
        else
            addToHead(node, idx);
        cache_size[idx] += value;
    }
    void evict(long long int key, long long int value, int idx)
    {
        while (cache_size[idx] + value > capacity[idx])
        {
            DLinkedNode *removed = removeTail(idx);
            cache_size[idx] -= removed->value;
            cache[idx].erase(removed->key);
            delete removed;
        }
    }
    //供外部调用
    void visit(long long int key, long long int value)
    {
        if (total_num == 11334)
        {
            int x = 1;
            x = 2;
        }
        total_num++;
        for (int i = 2; i >= 0; i--)
        {
            if (get(key, i))
            {
                if (i == 0)
                    hit_num++;
                moveToHead(cache[i][key], i); //promotion
            }
            else
            {
                if (flag)
                {
                    if (i == 1) //LRU miss则计数器+1
                    {
                        psel = (unsigned short)min(UINT16_MAX, psel + 1);
                    }
                    else if (i == 2) //BIP miss则计数器-1
                    {
                        psel = (unsigned short)max(0, psel - 1);
                    }
                }
                if (cache_size[i] + value <= capacity[i])
                {
                    set(key, value, i); //insertion
                }
                else //能够发生淘汰就代表预热结束了
                {
                    flag = true;
                    evict(key, value, i); //evict
                    set(key, value, i);   //insertion
                }
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

    void addToTail(DLinkedNode *node, int idx)
    {
        node->next = tail[idx];
        node->prev = tail[idx]->prev;
        tail[idx]->prev->next = node;
        tail[idx]->prev = node;
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

    ~DIPCache()
    {
        for (int i = 0; i < 3; i++)
        {
            for (pair<int, DLinkedNode *> block : cache[i])
                delete block.second;
            delete head[i];
            delete tail[i];
        }
    }
};

int main(int argc, char *argv[])
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter DIPCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    DIPCache cache(capa);

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
        if (value > capa)
        {
            printf("缓存块 %lld 的大小为 %lld , 大于缓存容量 %lld\n", key, value, capa);
            exit(1);
        }

        cache.visit(key, value);
    }
    cout << "-------DIP替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.total_num << ", 命中次数: " << cache.hit_num << endl;
    cout << "-------DIP替换算法-------" << endl;
}