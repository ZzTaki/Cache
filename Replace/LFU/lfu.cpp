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
using namespace std;

// 参考了 LeetCode 的官方题解，但有所不同的是，我实现的 LFU 算法是系统级的，而不是CPU片上级别
// 作者：LeetCode-Solution
// 链接：https://leetcode-cn.com/problems/lfu-cache/solution/lfuhuan-cun-by-leetcode-solution/
// 来源：力扣（LeetCode）
// 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。

// 缓存的节点信息
struct Node
{
    long long int key, val, freq;
    Node(long long int _key, long long int _val, long long int _freq) : key(_key), val(_val), freq(_freq) {}
};
class LFUCache
{
private:
    long long int capacity, hit_num = 0, total_num = 0;
    long long int cache_size = 0, minfreq = 0;
    unordered_map<long long int, list<Node>::iterator> key_table;
    unordered_map<long long int, list<Node>> freq_table;
    set<long long int> freq_set;

public:
    LFUCache(long long int _capacity) : capacity(_capacity)
    {
        key_table.clear();
        freq_table.clear();
        freq_set.clear();
    }

    // 外部调用
    void visit(long long int key, long long int value)
    {
        total_num++;
        if (!get(key))
        {
            if (value > capacity)
                return;
            Set(key, value);
        }
        else
        {
            hit_num++;
            promotion(key);
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
    bool get(long long int key) const
    {
        auto it = key_table.find(key);
        if (it == key_table.end())
            return false;
        else
            return true;
    }

    // 未命中时进行插入，如果缓存已满则还会进行淘汰
    void Set(long long int key, long long int value)
    {
        // 缓存已满，需要进行删除操作
        while (cache_size + value > capacity)
        {
            // 通过 minfreq 拿到 freq_table[minfreq] 链表的末尾节点
            auto it2 = freq_table[minfreq].back();
            // 删除频率最低的节点，如果删除后最低频率变化，则还会更新 freq_set 和 minfreq 以及在 freq_table 删除该频率链表
            cache_size -= it2.val;
            key_table.erase(it2.key);
            freq_table[minfreq].pop_back();
            if (freq_table[minfreq].size() == 0)
            {
                freq_table.erase(minfreq);
                freq_set.erase(minfreq);
                minfreq = *(freq_set.cbegin());
            }
        }
        // 加入新对象，频率为1
        cache_size += value;
        freq_table[1].push_front(Node(key, value, 1));
        key_table[key] = freq_table[1].begin();
        minfreq = 1;
        freq_set.insert(1);
    }

    // 命中，将命中对象频率提升1
    void promotion(long long int key)
    {
        auto it = key_table.find(key);
        list<Node>::iterator node = it->second;
        long long int val = node->val, freq = node->freq;
        freq_table[freq].erase(node);
        if (freq_table[freq].size() == 0)
        {
            freq_table.erase(freq);
            freq_set.erase(freq);
            if (minfreq == freq)
            {
                minfreq += 1;
            }
        }
        freq_table[freq + 1].push_front(Node(key, val, freq + 1));
        freq_set.insert(freq + 1);
        key_table[key] = freq_table[freq + 1].begin();
    }
};

int main()
{
    string line, temp;
    stringstream splitline;

    //交互
    long long int capa;
    cout << "Enter LFUCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    fstream fin_test(test);
    LFUCache cache(capa);

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
    cout << "-------LFU替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------LFU替换算法-------" << endl;

    fin_test.close();
}
