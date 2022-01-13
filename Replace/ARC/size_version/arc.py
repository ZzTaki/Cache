import collections


class ARCCache(object):
    def __init__(self, capacity):
        self.capacity = capacity
        self.size_t1 = 0
        self.size_t2 = 0
        self.size_b1 = 0
        self.size_b2 = 0
        self.map = {}  # {id: value}
        self.t1 = collections.OrderedDict(dict())
        self.t2 = collections.OrderedDict(dict())
        self.b1 = collections.OrderedDict(dict())
        self.b2 = collections.OrderedDict(dict())
        self.p = capacity / 2  # t1的最大空间

    def get(self, id):
        return id in cache.map

    def visit(self, id, value):
        if id in self.t1 or id in self.t2:  # 命中
            if id in self.t1:
                del self.t1[id]
                self.size_t1 -= self.map[id]
                self.t2[id] = self.map[id]
                self.size_t2 += self.map[id]  # 将id从t1移至t2的MRU
                self.my_replace()
            else:
                del self.t2[id]
                self.t2[id] = self.map[id]  # 将id从t2移至t2的MRU
        elif id in self.b1:  # 过去访问过一次后删除，说明t1小了
            self.p = min(
                self.p + max(self.b1[id], self.size_b2 // self.size_b1), self.capacity,
            )
            self.size_b1 -= self.b1[id]
            self.t2[id] = self.b1[id]
            self.size_t2 += self.b1[id]  # 将id从b1移至t2的MRU
            self.map[id] = self.b1[id]
            del self.b1[id]
            self.my_replace()
        elif id in self.b2:  # 过去访问过多次后删除，说明t2小了
            self.p = max(self.p - max(self.size_b1 // self.size_b2, self.b2[id]), 0)
            self.size_b2 -= self.b2[id]
            self.t2[id] = self.b2[id]
            self.size_t2 += self.b2[id]  # 将id从b2移至t2的MRU
            self.map[id] = self.b2[id]
            del self.b2[id]
            self.my_replace()
        else:  # 第一次访问
            if value > self.capacity:  # 请求文件的大小比缓存总容量还大
                return
            self.t1[id] = value
            self.size_t1 += value
            self.map[id] = value
            self.my_replace()

    def my_replace(self):
        # 检查 t1 和 b1
        while self.size_t1 > self.p:
            id, value = self.t1.popitem(last=False)
            self.b1[id] = value
            self.size_b1 += value
            self.size_t1 -= value
            del self.map[id]
        while self.size_b1 > 0 and self.size_b1 + self.size_t1 > self.capacity:
            id, value = self.b1.popitem(last=False)
            self.size_b1 -= value

        # 检查 t2 和 b2
        while self.size_t2 > self.capacity - self.p:
            id, value = self.t2.popitem(last=False)
            self.b2[id] = value
            self.size_b2 += value
            self.size_t2 -= value
            del self.map[id]
        while self.size_b2 > 0 and self.size_b2 + self.size_t2 > self.capacity:
            id, value = self.b2.popitem(last=False)
            self.size_b2 -= value


print("Enter ARCCache Capacity (Bytes) : ", end="")
capa = int(input())
print("Enter <Your Test File>: ", end="")
file = input()
cache = ARCCache(capa)
with open(file, "r", encoding="utf-8") as f:
    all_visit = 0
    hit = 0
    line = f.readline()
    while line:
        temp = line.split()
        value = int(temp[0])
        id = int(temp[1])
        if cache.get(id):
            hit += 1
        cache.visit(id, value)
        all_visit += 1
        line = f.readline()
    print("-------ARC替换算法-------")
    print("缓存容量 (Bytes) : " + str(capa))
    print("总请求数: " + str(all_visit) + ", 命中次数: " + str(hit))
    print("-------ARC替换算法-------")
f.close()
