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

class MINCache //opt算法占用空间开销大，因此 key 和 value 只设置为 int 类型了
{
private:
    long long int capacity, cache_size = 0;
    long long int total_num = 0, hit_num = 0;
    unordered_map<int, int> cache;              //id->size
    unordered_map<int, queue<int>> &id_indexes; //id: 出现在哪些行

public:
    MINCache(long long int _capacity, unordered_map<int, queue<int>> &_id_indexes) : capacity(_capacity), id_indexes(_id_indexes)
    {
        cache.clear();
    }

    void visit(int key, int value)
    {
        total_num++;
        id_indexes[key].pop();

        if (cache.count(key))
        {
            hit_num++;
        }
        else
        {
            if (value > capacity)
                return;
            cache[key] = value;
            cache_size += value;
            while (cache_size > capacity)
            {
                int evicted = searchMaxReuseId();
                cache_size -= cache[evicted];
                cache.erase(evicted);
            }
        }
        cout << total_num << endl;
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
    int searchMaxReuseId() const
    {
        int max_line = -1, evicted = -1;
        for (const pair<int, int> &block : cache) //选择前向重用距离最大的id
        {
            int id = block.first;
            const queue<int> &q = id_indexes.at(id);
            if (q.front() > max_line)
            {
                max_line = q.front();
                evicted = id;
            }
        }
        return evicted;
    }
};

void makeReusePos(string file_in, string file_out)
{
    fstream fin(file_in);
    string line, temp;
    stringstream splitline;
    unordered_map<int, vector<int>> reusePos;
    reusePos.clear();
    int idx = 1;
    while (getline(fin, line)) //遍历每个文件出现的行索引，加入到id_indexes[key]中
    {
        vector<string> data;
        splitline.clear();
        splitline.str(line);
        while (getline(splitline, temp, ' '))
        {
            data.push_back(temp);
        }
        int key = stol(data[1]);
        if (!reusePos.count(key))
        {
            reusePos[key] = vector<int>();
        }
        reusePos[key].push_back(idx);
        idx++;
    }
    string tmp_file = file_out + ".tmp";
    ofstream fout(tmp_file);
    for (const pair<int, vector<int>> &pr : reusePos) //将每个id出现的行位置输出到文件中
    {
        fout << pr.first << " : ";
        for (int pos : pr.second)
        {
            fout << pos << " ";
        }
        fout << endl;
    }
    fin.close();
    fout.close();
    if (rename(tmp_file.c_str(), file_out.c_str()))
    {
        cout << "Exception in renaming file from " << tmp_file << " to " << file_out << " code: " << strerror(errno) << endl;
        exit(-1);
    }
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
    unordered_map<int, queue<int>> id_indexes;
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
        int key = stol(data[0]);
        id_indexes[key] = queue<int>();
        for (int i = 2; i < data.size(); i++)
        {
            id_indexes[key].push(stol(data[i]));
        }
        id_indexes[key].push(INT32_MAX); //最后加一个无穷大
    }
    cout << id_indexes[32].size() << endl;

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
        int key = stol(data[1]), value = stol(data[0]);

        cache.visit(key, value);
    }
    cout << "-------MIN替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------MIN替换算法-------" << endl;

    f_index.close();
    fin_test.close();
}
