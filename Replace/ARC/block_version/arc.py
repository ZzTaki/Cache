import collections


class ARCCache(object):
    def __init__(self, capacity):
        self.capacity = capacity
        self.size_t1 = 0
        self.size_t2 = 0
        self.size_b1 = 0
        self.size_b2 = 0
        self.map = {}  # {id: True}
        self.t1 = collections.OrderedDict(dict())
        self.t2 = collections.OrderedDict(dict())
        self.b1 = collections.OrderedDict(dict())
        self.b2 = collections.OrderedDict(dict())
        self.p = 0  # t1的最大空间

    def visit(self, id):
        if id in self.t1 or id in self.t2:  # 命中
            if id in self.t1:
                del self.t1[id]
                self.size_t1 -= 1
                self.t2[id] = True
                self.size_t2 += 1  # 将id从t1移至t2的MRU
            else:
                del self.t2[id]
                self.t2[id] = True  # 将id从t2移至t2的MRU
        elif id in self.b1:  # 过去访问过一次后删除，说明t1小了
            if self.size_b1 >= self.size_b2:
                self.p = min(self.p + 1, self.capacity)
            else:
                self.p = min(self.p + self.size_b2 // self.size_b1, self.capacity)
            self.my_replace(id)
            del self.b1[id]
            self.size_b1 -= 1
            self.t2[id] = True
            self.size_t2 += 1  # 将id从b1移至t2的MRU
            self.map[id] = True
        elif id in self.b2:  # 过去访问过多次后删除，说明t2小了
            if self.size_b2 >= self.size_b1:
                self.p = max(self.p - 1, 0)
            else:
                self.p = max(self.p - self.size_b1 // self.size_b2, 0)
            self.my_replace(id)
            del self.b2[id]
            self.size_b2 -= 1
            self.t2[id] = True
            self.size_t2 += 1  # 将id从b2移至t2的MRU
            self.map[id] = True
        else:  # 第一次访问
            if self.size_t1 + self.size_b1 == self.capacity:
                if self.size_t1 < self.capacity:
                    self.b1.popitem(last=False)
                    self.size_b1 -= 1
                    self.my_replace(id)
                else:
                    key, value = self.t1.popitem(last=False)
                    self.size_t1 -= 1
                    del self.map[key]
            elif self.size_t1 + self.size_b1 < self.capacity:
                if (
                    self.size_t1 + self.size_t2 + self.size_b1 + self.size_b2
                    >= self.capacity
                ):
                    if (
                        self.size_t1 + self.size_t2 + self.size_b1 + self.size_b2
                        == 2 * self.capacity
                    ):
                        self.b2.popitem(last=False)
                        self.size_b2 -= 1
                    self.my_replace(id)
            self.t1[id] = True
            self.size_t1 += 1
            self.map[id] = True

    def my_replace(self, id):  # 删除t1或t2 LRU端多余的缓存（只在缓存空间满了，且有新数据加入缓存系统前才会调用）
        # 若想删除t1的LRU端，则需保证：（1）t1里面有元素；（2）t1里面的数量大于期望的大小p；或者t1里的数量达到p并且新访问数据在b2里)
        # (这意味着 |t1|<p 时，更希望将t1多填一点，因此智慧删除t2的LRU，如果 |t1|=p，则得看新访问的是否在b2里，如果在b2里，说明t2可能小了，也就只能删t1的LRU了)
        if self.size_t1 > 0 and (
            self.size_t1 > self.p or (id in self.b2 and self.size_t1 == self.p)
        ):
            key, value = self.t1.popitem(last=False)
            self.size_t1 -= 1
            del self.map[key]
            self.b1[key] = True
            self.size_b1 += 1  # 将t1的LRU移至b1
        else:
            key, value = self.t2.popitem(last=False)
            self.size_t2 -= 1
            del self.map[key]
            self.b2[key] = True
            self.size_b2 += 1  # 将t2的LRU移至b2


print("Enter ARCCache Capacity (Blocks) : ", end="")
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
        id = int(temp[1])
        if id in cache.map:
            hit += 1
        cache.visit(id)
        all_visit += 1
        line = f.readline()
    print("-------ARC替换算法-------")
    print("缓存容量 (Blocks) : " + str(capa))
    print("总请求数: " + str(all_visit) + ", 命中次数: " + str(hit))
    print("-------ARC替换算法-------")
f.close()
