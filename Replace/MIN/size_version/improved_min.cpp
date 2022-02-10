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
#include <map>
#include <cassert>
using namespace std;

class MINCache //opt算法占用空间开销大，因此 key 和 value 只设置为 int 类型了
{
private:
    long long int capacity, cache_size = 0;
    long long int total_num = 0, hit_num = 0;
    unordered_map<int, int> cache;               //id->size
    multimap<int, int, greater<int>> nextPos_id; //id: 出现在哪些行

public:
    MINCache(long long int _capacity) : capacity(_capacity)
    {
        cache.clear();
    }

    void visit(int key, int value, int nextPos)
    {
        nextPos_id.erase(total_num);

        if (cache.count(key))
        {
            hit_num++;
        }
        else
        {
            if (value <= capacity)
            {
                while (cache_size + value > capacity)
                {
                    auto maxReuseDis = nextPos_id.begin();
                    auto evicted = cache.find(maxReuseDis->second);
                    if (evicted != cache.end())
                    {
                        cache_size -= evicted->second;
                        cache.erase(evicted);
                    }
                    nextPos_id.erase(maxReuseDis);
                }
                cache[key] = value;
                cache_size += value;
            }
        }
        nextPos_id.emplace(nextPos, key);
        total_num++;
        cout << total_num << endl;
        assert(nextPos_id.size() == cache.size());
    }

    long long int getTotal() const
    {
        return total_num;
    }

    long long int getHit() const
    {
        return hit_num;
    }
};

void makeReusePos(string file_in, string file_out)
{
    fstream fin(file_in);
    string line, temp;
    stringstream splitline;
    vector<int> idTrace;
    idTrace.clear();
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
        idTrace.push_back(key);
        idx++;
    }

    unordered_map<int, int> id_lastPos;
    vector<int> next_pos(idTrace.size(), 0);

    for (int i = idTrace.size(); i >= 0; i--)
    {
        int current_id = idTrace[i];
        auto lastIt = id_lastPos.find(current_id);
        if (lastIt != id_lastPos.end())
        {
            next_pos[i] = lastIt->second;
        }
        else
        {
            next_pos[i] = INT32_MAX;
        }
        id_lastPos[current_id] = i;
    }

    fin.clear();
    fin.seekg(0, ios::beg);
    int value, key, yewu;
    string tmp_file = file_out + ".tmp";
    ofstream fout(tmp_file);
    for (int i = 0; i < idTrace.size(); i++) //为trace添加附加信息：当前请求的id下次出现的位置
    {
        fin >> value >> key >> yewu;
        fout << next_pos[i] << " " << value << " " << key << " " << yewu << endl; //next_pos id
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
    string line;
    stringstream splitline;
    long long int capa;
    cout << "Enter MINCache Capacity (Bytes) : ";
    cin >> capa;
    string test;
    cout << "Enter <Your Test File>: ";
    cin >> test;

    cout << "Making NextPosTrace File..." << endl;
    ifstream f(test + ".npt");
    if (!f.good()) //检查文件是否存在
    {
        makeReusePos(test, test + ".npt");
    }
    cout << "NextPosTrace File Complete" << endl;

    fstream f_npt(test + ".npt");

    MINCache cache(capa);

    while (getline(f_npt, line))
    {
        splitline.clear();
        splitline.str(line);
        //检查是否有某个块比缓存容量还大
        int nextPos, value, key, yewu;
        splitline >> nextPos >> value >> key >> yewu;

        cache.visit(key, value, nextPos);
    }
    cout << "-------MIN替换算法-------" << endl;
    cout << "缓存容量 (Bytes) ：" << capa << endl;
    cout << "总请求数: " << cache.getTotal() << ", 命中次数: " << cache.getHit() << endl;
    cout << "-------MIN替换算法-------" << endl;
}
