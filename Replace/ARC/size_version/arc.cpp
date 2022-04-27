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
class ARCCache
{
    DLinkedNode *headt1, *headt2, *headb1, *headb2;
    DLinkedNode *tailt1, *tailt2, *tailb1, *tailb2;
    long long int capacity;
    unordered_map<long long int, DLinkedNode *> t1, t2, b1, b2; // id->{id, value, prev, next}
    long long int size_t1 = 0, size_t2 = 0, size_b1 = 0, size_b2 = 0;
    long long int hit_num, total_num, p;

public:
    ARCCache(long long int capacity)
    {
        t1.clear();
        t2.clear();
        b1.clear();
        b2.clear();
        this->capacity = capacity;
        this->p = capacity / 2;
        headt1 = new DLinkedNode(), headt2 = new DLinkedNode(), headb1 = new DLinkedNode(), headb2 = new DLinkedNode();
        tailt1 = new DLinkedNode(), tailt2 = new DLinkedNode(), tailb1 = new DLinkedNode(), tailb2 = new DLinkedNode();
        headt1->next = tailt1, tailt1->prev = headt1;
        headt2->next = tailt2, tailt2->prev = headt2;
        headb1->next = tailb1, tailb1->prev = headb1;
        headb2->next = tailb2, tailb2->prev = headb2;
        hit_num = total_num = 0;
    }
    ~ARCCache()
    {
        for (pair<const long long int, DLinkedNode *> &block : t1)
            delete block.second;
        for (pair<const long long int, DLinkedNode *> &block : t2)
            delete block.second;
        for (pair<const long long int, DLinkedNode *> &block : b1)
            delete block.second;
        for (pair<const long long int, DLinkedNode *> &block : b2)
            delete block.second;
        delete headt1;
        delete headt2;
        delete headb1;
        delete headb2;
        delete tailt1;
        delete tailt2;
        delete tailb1;
        delete tailb2;
    }
    void visit(long long int key, long long int value)
    {
        total_num += value;
        if (get(key)) //命中
        {
            hit_num += value;
            if (t1.count(key))
            {
                DLinkedNode *moved = t1[key];
                size_t1 -= moved->value;
                t1.erase(key);
                t2[key] = moved;
                size_t2 += moved->value;
                moveToHead(moved, headt2); //将key从t1移至t2的MRU
                my_replace();
            }
            else
            {
                moveToHead(t2[key], headt2);
            }
        }
        else if (b1.count(key)) //过去访问过一次后删除，说明t1小了
        {
            p = min(p + max(b1[key]->value, size_b2 / size_b1), capacity);
            DLinkedNode *moved = b1[key];
            size_b1 -= moved->value;
            b1.erase(key);
            t2[key] = moved;
            size_t2 += moved->value;
            moveToHead(moved, headt2); // 将key从b1移至t2的MRU
            my_replace();
        }
        else if (b2.count(key)) //过去访问过多次后删除，说明t2小了
        {
            p = max(p - max(b2[key]->value, size_b1 / size_b2), (long long int)0);
            DLinkedNode *moved = b2[key];
            size_b2 -= moved->value;
            b2.erase(key);
            t2[key] = moved;
            size_t2 += moved->value;
            moveToHead(moved, headt2); // 将key从b2移至t2的MRU
            my_replace();
        }
        else
        {
            if (value > capacity)
                return;
            set(key, value);
            my_replace();
        }
    }
    long long int getTotal() const { return total_num; }
    long long int getHit() const { return hit_num; }

private:
    bool get(long long int key)
    {
        return t1.count(key) || t2.count(key);
    }
    void set(long long int key, long long int value)
    {
        DLinkedNode *node = new DLinkedNode(key, value);
        t1[key] = node;
        addToHead(node, headt1);
        size_t1 += value;
    }

    void addToHead(DLinkedNode *node, DLinkedNode *head)
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

    void moveToHead(DLinkedNode *node, DLinkedNode *head)
    {
        removeNode(node);
        addToHead(node, head);
    }

    DLinkedNode *removeTail(DLinkedNode *tail)
    {
        DLinkedNode *node = tail->prev;
        removeNode(node);
        return node;
    }

    void my_replace()
    {
        while (size_t1 > p)
        {
            DLinkedNode *moved = removeTail(tailt1);
            size_t1 -= moved->value;
            t1.erase(moved->key);
            b1[moved->key] = moved;
            size_b1 += moved->value;
            moveToHead(moved, headb1);
        }
        while (size_b1 > 0 && size_b1 + size_t1 > capacity)
        {
            DLinkedNode *removed = removeTail(tailb1);
            size_b1 -= removed->value;
            b1.erase(removed->key);
            delete removed;
        }

        while (size_t2 > capacity - p)
        {
            DLinkedNode *moved = removeTail(tailt2);
            size_t2 -= moved->value;
            t2.erase(moved->key);
            b2[moved->key] = moved;
            size_b2 += moved->value;
            moveToHead(moved, headb2);
        }
        while (size_b2 > 0 && size_b2 + size_t2 > capacity)
        {
            DLinkedNode *removed = removeTail(tailb2);
            size_b2 -= removed->value;
            b2.erase(removed->key);
            delete removed;
        }
    }
};

int main(int argc, char *argv[])
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter ARCCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    ARCCache cache(capa);

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
    cout << "-------ARC替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------ARC替换算法-------" << endl;

    fin_test.close();
}