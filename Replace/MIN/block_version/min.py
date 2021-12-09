from collections import defaultdict, OrderedDict, deque

# 原论文中的版本


class MINCache(object):
    def __init__(self, capacity):
        self.capacity = capacity
        self.evicted = deque()  # 浏览一次访问流后得到输入集最大化命中率的替换顺序
        # {index: {id: True, id: True}} 记录缓存中各个序号有多少块
        self.request_index = defaultdict(OrderedDict)
        self.block = {}  # {id: index}
        self.complete = 1
        self.cur = 0

        self.map = {}  # {id: True} 真实缓存空间，在第二次访问流时才使用

    def train(self, id):
        # 该块未被访问过或已经被替换
        if id not in self.block or self.block[id] < self.complete:
            self.cur += 1
            self.block[id] = self.cur
            self.request_index[self.cur][id] = True
            return

        if self.block[id] == self.cur:  # 连续访问
            return

        if self.block[id] < self.cur and self.block[id] >= self.complete:  # 仍然在缓存中，且重用距离大于1
            self.request_index[self.block[id]].pop(id)
            if not self.request_index[self.block[id]]:
                del self.request_index[self.block[id]]
            self.block[id] = self.cur
            self.request_index[self.cur][id] = True

            temp = self.cur
            counter = 0

            while True:
                if self.request_index[temp]:
                    counter += len(self.request_index[temp])
                if counter == self.capacity or temp == self.complete:
                    self.complete = temp  # 索引小于complete的块消失
                    no_value = []
                    for idx, d in self.request_index.items():
                        if idx >= self.complete:
                            for t in no_value:
                                del self.request_index[t]
                            return
                        for evic, b in d.items():
                            self.evicted.append(evic)
                        no_value.append(idx)

                if counter < self.capacity:
                    temp -= 1
                    counter -= 1
                    continue

    def build_tail(self):
        for idx, d in self.request_index.items():
            for evic, b in d.items():
                self.evicted.append(evic)

    def visit(self, id):
        if self.capacity == 0:
            return

        if id in self.map:
            return

        if len(self.map) < self.capacity:
            self.map[id] = True
            return

        key = self.evicted.popleft()
        del self.map[key]
        self.map[id] = True


print("Enter MINCache Capacity (Blocks) : ", end='')
capa = int(input())
print("Enter <Your Test File>: ", end='')
file = input()
cache = MINCache(capa)

with open(file, "r", encoding="utf-8") as f:
    all_visit = 0
    hit = 0
    line = f.readline()
    while line:
        temp = line.split()
        id = int(temp[1])
        cache.train(id)
        line = f.readline()
    cache.build_tail()
f.close()

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
    print("-------MIN替换算法-------")
    print("缓存容量 (Blocks) : " + str(capa))
    print("总请求数: " + str(all_visit) + ", 命中次数: " + str(hit))
    print("-------MIN替换算法-------")
f.close()
