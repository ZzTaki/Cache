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
#include <queue>
#include <climits>
using namespace std;

class MINCache
{
public:
    long long int capacity, cache_size;
    long long int total_num, hit_num;
    unordered_map<long long int, long long int> cache;             //id->size
    unordered_map<long long int, queue<long long int>> id_indexes; //id: 出现在哪些行
    MINCache(long long int _capacity, unordered_map<long long int, queue<long long int>> &_id_indexes)
    {
        capacity = _capacity;
        cache_size = total_num = hit_num = 0;
        cache.clear();
        id_indexes.clear();
        id_indexes = _id_indexes;
    }

    void visit(long long int key, long long int value)
    {
        id_indexes[key].pop();
        total_num++;
        if (cache.count(key))
        {
            hit_num++;
        }
        else
        {
            if (value > capacity)
                return;
            while (cache_size + value > capacity)
            {
                long long int evicted = searchMaxReuseId();
                cache_size -= cache[evicted];
                cache.erase(evicted);
            }
            cache[key] = value;
            cache_size += value;
        }
    }

    long long int searchMaxReuseId()
    {
        long long int max_line = -1, evicted = -1;
        for (const pair<long long int, long long int> &block : cache) //选择前向重用距离最大的id
        {
            long long int id = block.first;
            if (id_indexes[id].front() > max_line)
            {
                max_line = id_indexes[id].front();
                evicted = id;
            }
        }
        return evicted;
    }
};

void makeReusePos(string file_in, string file_out)
{
    fstream fin(file_in);
    ofstream fout(file_out);
    string line, temp;
    stringstream splitline;
    unordered_map<long long int, vector<long long int>> reusePos;
    reusePos.clear();
    long long int idx = 1;
    while (getline(fin, line)) //遍历每个文件出现的行索引，加入到id_indexes[key]中
    {
        vector<string> data;
        splitline.clear();
        splitline.str(line);
        while (getline(splitline, temp, ' '))
        {
            data.push_back(temp);
        }
        long long int key = stoll(data[1]);
        if (!reusePos.count(key))
        {
            reusePos[key] = vector<long long int>();
        }
        reusePos[key].push_back(idx);
        idx++;
    }
    for (const pair<long long int, vector<long long int>> &pr : reusePos) //将每个id出现的行位置输出到文件中
    {
        fout << pr.first << " : ";
        for (long long int pos : pr.second)
        {
            fout << pos << " ";
        }
        fout << endl;
    }
    fin.close();
    fout.close();
}

int main()
{
    string line, temp;
    stringstream splitline;

    long long int capa;
    cout << "Enter MINCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    cout << "Making ReusePosition File..." << endl;
    ifstream f(test + ".pos");
    if (!f.good()) //检查文件是否存在
    {
        makeReusePos(test, test + ".pos");
    }
    cout << "ReusePosition File Complete" << endl;

    fstream f_index(test + ".pos");
    unordered_map<long long int, queue<long long int>> id_indexes;
    id_indexes.clear();
    while (getline(f_index, line)) //遍历每个文件出现的行索引，加入到id_indexes[key]中
    {
        vector<string> data;
        splitline.clear();
        splitline.str(line);
        while (getline(splitline, temp, ' '))
        {
            data.push_back(temp);
        }
        long long int key = stoll(data[0]);
        id_indexes[key] = queue<long long int>();
        for (int i = 2; i < data.size(); i++)
        {
            id_indexes[key].push(stoll(data[i]));
        }
        id_indexes[key].push(LLONG_MAX); //最后加一个无穷大
    }

    fstream fin_test(test);
    MINCache cache(capa, id_indexes);

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
    cout << "-------MIN替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.total_num << ", 命中次数: " << cache.hit_num << endl;
    cout << "-------MIN替换算法-------" << endl;
}
