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
    bool flag;
    DLinkedNode *prev;
    DLinkedNode *next;
    DLinkedNode() : key(0), value(0), flag(false), prev(nullptr), next(nullptr) {}
    DLinkedNode(long long int _key, long long int _value, bool _flag) : key(_key), value(_value), flag(_flag), prev(nullptr), next(nullptr) {}
};
class DLIRSCache //DLIRS使用和缓存容量相等的 shadow cache
{
private:
    DLinkedNode *head[2], *tail[2]; //0控制s，1控制q
    unordered_map<long long int, DLinkedNode *> s;
    unordered_map<long long int, DLinkedNode *> q;
    unordered_map<long long int, int> key_state; // {id : x} (x = 0, 1, 2)  0代表LIR，1代表resident HIR, 2代表 non-resident HIR
    long long int total_num = 0, hit_num = 0;

    long long int t_l, t_hr, size_l = 0, size_hr = 0, size_hn = 0, size_hd = 0; //期望LIR容量、期望resdent hir容量、当前LIR容量、当前resident hir容量、当前non-resident hir所占shadow cache容量、当前从LIR状态降级为resident hir的文件所占容量
    long long int capacity;

public:
    DLIRSCache(long long int _capacity, double rate = 9.0 / 10) : capacity(_capacity), t_l(capacity * rate), t_hr(capacity - t_l)
    {
        head[0] = new DLinkedNode(), head[1] = new DLinkedNode();
        tail[0] = new DLinkedNode(), tail[1] = new DLinkedNode();
        head[0]->next = tail[0], tail[0]->prev = head[0];
        head[1]->next = tail[1], tail[1]->prev = head[1];
        s.clear();
        q.clear();
        key_state.clear();
    }

    ~DLIRSCache()
    {
        for (pair<const long long int, DLinkedNode *> &p : s)
        {
            delete p.second;
            p.second = nullptr;
        }
        for (pair<const long long int, DLinkedNode *> &p : q)
        {
            delete p.second;
            p.second = nullptr;
        }
        delete head[0];
        head[0] = nullptr;
        delete head[1];
        head[1] = nullptr;
        delete tail[0];
        tail[0] = nullptr;
        delete tail[1];
        tail[1] = nullptr;
    }

    void visit(long long int key, long long int value)
    {
        total_num++;
        if (key_state.count(key) && key_state[key] == 0) //访问热数据
        {
            hit_num++;
            //热数据只需移到MRU即可，并对LRU端进行热链循环剪枝
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
                size_l += moved->value;

                //删除请求块在q中的数据
                DLinkedNode *removed = q[key];
                size_hr -= removed->value;
                q.erase(key);
                removeNode(removed);
                delete removed;
                removed = nullptr;

                bringCurrentBackToTarget();
            }
            else //常驻冷数据在 s 中没有索引
            {
                //创建索引添加到 s 中
                DLinkedNode *node = new DLinkedNode(key, value, false);
                addToHead(node, 0);
                s[key] = node;

                //将常驻冷数据移动到 q 的MRU
                DLinkedNode *moved = q[key];
                moveToHead(moved, 1);
                if (moved->flag)
                {
                    t_l = min(capacity, t_l + max(moved->value, size_hn / size_hd));
                    t_hr = capacity - t_l;
                    size_hd -= moved->value;
                    moved->flag = false;
                }
            }
        }
        else //不在缓存中
        {
            if (key_state.count(key) && key_state[key] == 2) //但在 s 中有索引，直接提升为热数据
            {
                //将在 s 中有索引的非常驻冷数据块置热、移动到 s 的MRU
                key_state[key] = 0;
                DLinkedNode *moved = s[key];
                moveToHead(moved, 0);
                t_hr = min(capacity, t_hr + max(moved->value, size_hd / size_hn));
                t_l = capacity - size_hr;
                size_hn -= moved->value, size_l += moved->value;
                bringCurrentBackToTarget();
            }
            else
            {
                if (size_hr == 0 && size_l + value <= t_l) //初始状态，先填热链
                {
                    key_state[key] = 0;
                    DLinkedNode *node_s = new DLinkedNode(key, value, false);
                    addToHead(node_s, 0);
                    s[key] = node_s;
                    size_l += value;
                }
                else //填冷链
                {
                    if (value > capacity)
                        return;
                    key_state[key] = 1;
                    DLinkedNode *node_s = new DLinkedNode(key, value, false);
                    DLinkedNode *node_q = new DLinkedNode(key, value, false);
                    addToHead(node_s, 0);
                    addToHead(node_q, 1);
                    s[key] = node_s;
                    q[key] = node_q;
                    size_hr += value;
                    bringCurrentBackToTarget();
                }
                if (size_l + size_hr + size_hn > 2 * capacity)
                {
                    long long int count = 0; //统计删除的非常驻冷数据索引的大小总和
                    DLinkedNode *cur = tail[0]->prev;
                    while (cur != head[0] && size_l + size_hr + size_hn - count > 2 * capacity)
                    {
                        if (key_state[cur->key] == 2)
                            count += cur->value;
                        cur = cur->prev;
                    }
                    DLinkedNode *removed = tail[0]->prev;
                    while (removed != cur)
                    {
                        DLinkedNode *tmp = removed->prev;
                        if (key_state[removed->key] != 2)
                        {
                            removed = tmp;
                        }
                        else
                        {
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
                    size_hn -= count;
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

    void bringCurrentBackToTarget()
    {
        while (size_l > t_l)
        {
            //从s中删除LRU元素
            DLinkedNode *moved = removeTail(0);
            s.erase(moved->key);
            moved->flag = true;

            //将该元素加入q的MRU
            addToHead(moved, 1);
            q[moved->key] = moved;
            key_state[moved->key] = 1;

            size_l -= moved->value;
            size_hr += moved->value;
            size_hd += moved->value;
            cut();
        }
        while (size_hr > t_hr)
        {
            DLinkedNode *removed = removeTail(1);
            q.erase(removed->key);
            if (!s.count(removed->key))
            {
                key_state.erase(removed->key);
            }
            else
            {
                key_state[removed->key] = 2;
                size_hn += removed->value;
            }
            size_hr -= removed->value;
            if (removed->flag)
                size_hd -= removed->value;
            delete removed;
            removed = nullptr;
        }
        return;
    }

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
                size_hn -= removed->value;
            }
            delete removed;
            removed = tmp;
        }
    }
};

int main()
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter DLIRSCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    DLIRSCache cache(capa);

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
    cout << "-------DLIRS替换算法-------" << endl;
    cout << "缓存容量 (Bytes) :" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------DLIRS替换算法-------" << endl;

    fin_test.close();
}
