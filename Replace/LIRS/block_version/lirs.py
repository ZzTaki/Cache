import collections


class LIRSCache(object):
    def __init__(self, capacity, rate=1/6):
        self.lhirs = int(capacity * rate)
        self.llirs = capacity - self.lhirs
        self.size_lirs = 0
        self.size_q = 0
        # 若map中不存在某个键值，可以视为该id对应的值为2
        # {id: x} (x = 0, 1, 2) 0代表LIR，1代表resident HIR, 2代表 non-resident HIR
        self.map = {}
        # 负责热数据（LIR）的淘汰，在s中不代表就在cache中
        self.s = collections.OrderedDict(dict())
        # 负责冷数据（HIR）的淘汰 {id: True}，在q中就一定在cache中
        self.q = collections.OrderedDict(dict())

    def visit(self, id):
        if self.lhirs == 0 and self.llirs == 0:
            return

        if id not in self.map:  # 第一次读，若直接return，说明处于初始化阶段
            if self.llirs != self.size_lirs:
                self.s[id] = True
                self.size_lirs += 1
                self.map[id] = 0
                return
            else:
                if self.lhirs != self.size_q:
                    self.s[id] = True  # 注意这里
                    self.q[id] = True
                    self.size_q += 1
                    self.map[id] = 1
                    return

        s_list = list(self.s.keys())
        if id in self.s and self.map[id] == 0:  # 访问s中的LIR块
            self.s.pop(id)
            self.s[id] = True  # 移动到栈顶
            if id == s_list[0]:  # 如果原本这个LIR在栈底，则进行剪枝
                self.cut()
        elif id in self.map and self.map[id] == 1:  # 访问一个residnet HIR块
            if id in self.s:  # 在栈s中
                self.s.pop(id)
                self.s[id] = True  # id移至栈顶
                self.map[id] = 0  # 更新为LIR
                self.q.pop(id)  # 从队列中删除id
                self.map[s_list[0]] = 1
                self.q[s_list[0]] = True  # s栈底元素加入队列q
                self.cut()
            else:  # 不在栈s中
                self.s[id] = True  # 加入栈s
                self.q.pop(id)
                self.q[id] = True  # 在队列的位置更改为队尾
        else:  # id不在cache中
            key, value = self.q.popitem(last=False)
            self.map[key] = 2  # 从队列q中删除首元素，更改状态为NON-HIR
            if id in self.s:  # id在栈s中
                self.map[id] = 0  # id更新为LIR
                self.s.pop(id)
                self.s[id] = True  # 将id移至栈顶
                self.map[s_list[0]] = 1
                self.q[s_list[0]] = True  # s栈底元素加入队列q
                self.cut()
            else:  # id不在s中
                self.map[id] = 1  # 加入cache
                self.s[id] = True
                self.q[id] = True  # 加入栈顶和队尾

    def cut(self):
        i = 0
        s_list = list(self.s.keys())
        while i < len(s_list):
            if self.map[s_list[i]] != 0:
                i += 1
            else:
                break
        for j in range(i):
            del self.s[s_list[j]]


print("Enter LIRSCache Capacity (Blocks) : ", end='')
capa = int(input())
print("Enter <Your Test File>: ", end='')
file = input()
cache = LIRSCache(capa)
with open(file, "r", encoding="utf-8") as f:
    all_visit = 0
    hit = 0
    line = f.readline()
    while line:
        temp = line.split()
        id = int(temp[1])
        if id in cache.map and cache.map[id] != 2:
            hit += 1
        cache.visit(id)
        all_visit += 1
        line = f.readline()
    print("-------LIRS替换算法-------")
    print("缓存容量 (Blocks) : " + str(capa))
    print("总请求数: " + str(all_visit) + ", 命中次数: " + str(hit))
    print("-------LIRS替换算法-------")
f.close()
