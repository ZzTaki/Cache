#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <list>
#include <assert.h>
using namespace std;

struct DLinkedNode
{
    long long int key, value;
    DLinkedNode *prev;
    DLinkedNode *next;
    DLinkedNode() : key(0), value(0), prev(nullptr), next(nullptr) {}
    DLinkedNode(long long int _key, long long int _value) : key(_key), value(_value), prev(nullptr), next(nullptr) {}
};
class LIRSCache //LIRS使用 max(Tlast-reuseDistance, Tlast) 作为衡量标准，该指标越小，代表数据越热。所有又有IRR，又有recency。
{
private:
    DLinkedNode *head[2], *tail[2]; //0控制s，1控制q
    unordered_map<long long int, DLinkedNode *> s;
    unordered_map<long long int, DLinkedNode *> q;
    unordered_map<long long int, int> key_state; // {id : x} (x = 0, 1, 2)  0代表LIR，1代表resident HIR, 2代表 non-resident HIR
    long long int capa_s, capa_q;
    long long int size_s, size_q;
    long long int total_num, hit_num;

public:
    LIRSCache(long long int capacity, double rate = 9.0 / 10)
    {
        head[0] = new DLinkedNode(), head[1] = new DLinkedNode();
        tail[0] = new DLinkedNode(), tail[1] = new DLinkedNode();
        head[0]->next = tail[0], tail[0]->prev = head[0];
        head[1]->next = tail[1], tail[1]->prev = head[1];
        s.clear();
        q.clear();
        key_state.clear();
        capa_s = capacity * rate;
        capa_q = capacity - capa_s;
        size_s = size_q = 0;
        total_num = hit_num = 0;
    }

    ~LIRSCache()
    {
        for (const pair<long long int, DLinkedNode *> &p : s)
        {
            delete p.second;
        }
        for (const pair<long long int, DLinkedNode *> &p : q)
        {
            delete p.second;
        }
        delete head[0];
        delete head[1];
        delete tail[0];
        delete tail[1];
    }

    void visit(long long int key, long long int value)
    {
        total_num++;
        if (key_state.count(key) && key_state[key] == 0) //访问热数据
        {
            hit_num++;
            //热数据只需移到s的MRU即可，并对s的LRU端进行热链循环剪枝
            DLinkedNode *moved = s[key];
            moveToHead(moved, 0);
            cut();
        }
        else if (key_state.count(key) && key_state[key] == 1) //常驻冷数据
        {
            hit_num++;
            if (s.count(key)) //常驻冷数据在 s 中有索引
            {
                //常驻冷数据置热、移至s的MRU、更新s缓存大小
                key_state[key] = 0;
                DLinkedNode *moved = s[key];
                moveToHead(moved, 0);
                size_s += moved->value;

                //删除请求块在q中的数据
                DLinkedNode *removed = q[key];
                size_q -= removed->value;
                q.erase(key);
                removeNode(removed);
                delete removed;
            }
            else //常驻冷数据在 s 中没有索引
            {
                //创建索引添加到 s 中
                DLinkedNode *node = new DLinkedNode(key, value);
                addToHead(node, 0);
                s[key] = node;

                //将常驻冷数据移动到 q 的MRU
                DLinkedNode *moved = q[key];
                moveToHead(moved, 1);
            }
        }
        else //不在缓存中
        {
            if (key_state.count(key) && key_state[key] == 2) //但在 s 中有索引，直接提升为热数据 (s.count(key)条件可以不加)
            {
                //将在 s 中有索引的非常驻冷数据块置热、移动到 s 的MRU
                key_state[key] = 0;
                DLinkedNode *moved = s[key];
                moveToHead(moved, 0);
                size_s += moved->value;
            }
            else
            {
                if (size_q == 0 && size_s + value <= capa_s)
                {
                    key_state[key] = 0;
                    DLinkedNode *node_s = new DLinkedNode(key, value);
                    addToHead(node_s, 0);
                    s[key] = node_s;
                    size_s += value;
                }
                else
                {
                    if (value > capa_q)
                        return;
                    key_state[key] = 1;
                    DLinkedNode *node_s = new DLinkedNode(key, value);
                    DLinkedNode *node_q = new DLinkedNode(key, value);
                    addToHead(node_s, 0);
                    addToHead(node_q, 1);
                    s[key] = node_s;
                    q[key] = node_q;
                    size_q += value;
                }
            }
        }
        while (size_s > capa_s)
        {
            //从s中删除LRU元素
            DLinkedNode *moved = removeTail(0);
            size_s -= moved->value;
            s.erase(moved->key);
            assert(key_state[moved->key] == 0);

            //将该元素加入q的栈底
            addToHead(moved, 1);
            size_q += moved->value;
            q[moved->key] = moved;
            key_state[moved->key] = 1;

            //热链循环剪枝
            cut();
        }
        while (size_q > capa_q)
        {
            //直接删除
            DLinkedNode *removed = removeTail(1);
            size_q -= removed->value;
            q.erase(removed->key);
            if (!s.count(removed->key)) //q中删除的文件在s中也没有索引，key_state也没有意义了
            {
                key_state.erase(removed->key);
            }
            else
            {
                key_state[removed->key] = 2; //变成非常驻冷数据
            }
            delete removed;
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
    void cut() //热循环剪枝
    {
        DLinkedNode *cur = tail[0]->prev;
        while (cur != head[0] && key_state[cur->key] != 0)
        {
            cur = cur->prev;
        }
        DLinkedNode *removed = tail[0]->prev;
        while (removed != cur)
        {
            DLinkedNode *tmp = removed->prev;
            s.erase(removed->key);
            removeNode(removed);
            if (key_state[removed->key] == 2) //非常驻冷数据
            {
                key_state.erase(removed->key);
            }
            delete removed;
            removed = tmp;
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

int main()
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter LIRSCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    LIRSCache cache(capa);

    while (getline(fin_test, line))
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
    cout << "-------LIRS替换算法-------" << endl;
    cout << "缓存容量 (Bytes) :" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------LIRS替换算法-------" << endl;
}
