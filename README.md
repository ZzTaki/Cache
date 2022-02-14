# Cache
## Cache Replace
假设 cache 平均容纳 m 个块，trace流有 n 个请求。

### Random
随机替换策略，模拟一个数组。
- insertion：当 cache miss 时 insert 新块到数组的末尾。
- eviction：当 insert 时如果 cache full，则从数组中随机选择若干块进行 evict，直到 cache 有足够空间容纳新块。
- promotion：无。
- 时间复杂度：O(n)

### FIFO
先进先出策略，模拟一个队列。
- insertion：当 cache miss 时 insert 新块到队列的末尾。
- eviction：当 insert 时如果 cache full，则 evict 队列首部若干块，直到 cache 有足够空间容纳新块。
- promotion：FIFO 没有 promotion 操作。
- idea：使用进入缓存的时间长短作为预测前向重用距离的指标，认为越早进入 cache 的前向重用距离越大。
- 时间复杂度：O(n)

### LRU
最近最少使用策略，模拟一个链表，并使用一个哈希表来保存 key->DlinkNode* 的映射，以便在 O(1) 时间复杂度内对 hit块 进行 promotion。
- insertion：当 cache miss 时 insert 新块到链表的头部。
- eviction：当 insert 时如果 cache full，则 evict 链表尾部若干块，直到 cache 有足够空间容纳新块。
- promotion：当 hit 时，将 hit块 从原位置提升到链表的头部。
- idea：使用 recency 作为预测前向重用距离的指标，认为 cache 中越久没有再被访问的前向重用距离越大。
- 时间复杂度：O(n)

### LRU-k
相对LRU有略微调整，增加一个历史表，只有文件块在历史表中被额外访问了 k-1 次才会加入缓存。
- insertion：当 cache miss 时，检查文件信息是否在历史表中，如果不在，则 insert 历史表，命中次数记为0；如果在，则将该文件历史表的命中次数+1，并移动到历史表的头部，当命中次数到达 k-1 次时，则从历史表中删除，并 insert 到真实缓存中的 MRU 端。
- eviction：当 insert 时如果历史表满了，或者 cache full，则 evict 历史表尾部的若干 shadow cache 块，evict LRU链表尾部的若干块。
- promotion：当 hit 历史表或者 hit cache，将命中块移动到历史表或LRU链表头部。
- idea：使用 recency 和 coarse-grained fluency 作为预测前向重用距离的指标，通过历史表可以在一定程度上减轻 scan 工作负载，但如果大量的块仅访问 2~(k-1) 次，会导致将本可能可以提供命中的块变成缺失块。 
- 时间复杂度：O(n)

### S3LRU
分成三段的LRU策略，模拟三条等长的LRU链表，依次为低级、中级和高级链表。
- insertion：当 cache miss 时 insert 新块到低级链表的头部。
- eviction：当低级链表容量不够时，则 evict 低级链表尾部的若干块，直到低级链表能够容纳新块。
- promotion：当 hit 时，如果 hit块 在低级和中级链表之中，则将 hit块 提升到高一级的链表的头部，如果 hit块 在高级链表之中，则将 hit块 提升到高级链表的头部。
- demotion：当中级或高级链表超过对应容量时，将中级或高级链表的尾部若干块降级到低一级的链表的头部。
- idea：使用 recency 和 coarse-grained fluency 作为预测前向重用距离的指标，通过分段LRU一定程度上减轻了短暂的 scan 工作负载对于 LRU cache 的影响，这有点像 LRU-k，但它过滤 scan 工作负载的方式还是使用部分真实的缓存，其余的用来保护频率相对较高的块；而 LRU-k 则是使用历史表 shadow cache，真实缓存全部作为保护。
- 时间复杂度：O(n)

### ARC
自适应替换策略，模拟两条长度可变的LRU链表，分别为 t1 和 t2，总容量为c，同时每条LRU链表还有一段历史表，分别为 b1 和 b2，用来存储最近从对应链表中evict出来的块的key，每条LRU链表和其历史表的容量总和也是c。
- insertion：当 cache miss 时，如果命中 b1 或 b2 历史表，则将历史表中该块的信息删除，并 insert 该块到 t2 的头部。
- 自适应调整：当 cache miss 时，如果命中 b1，说明 miss块 最近刚从 t1 中驱逐，如果增大 t1 的容量，可能该请求就命中了，于是增加 t1 的容量，减少 t2 的容量；如果命中 b2，说明 miss块 最近刚从 t2 中驱逐，如果增大 t2 的容量，可能该请求就命中了，于是增加 t2 的容量，减少 t1 的容量。
- promotion：当 hit 时，该 hit块 从原位置删除，并提升到 t2 的头部。
- eviction：当 t1 或 t2 full时，将 t1 或 t2 尾部若干块删除并添加到 b1 或 b2 中。
- idea：使用 recency 和 coarse-grained fluency 作为预测前向重用距离的指标，通过 LFU链表（t2）一定程度上减轻了短暂的 scan 工作负载对于 LRU cache 的影响，同时有自适应调整功能，相较于 S3LRU 和 LIRS 策略也可以更好地适应较长的 LRU-friendly 工作负载。
- 时间复杂度：O(n)

### LIRS
Low Inter-Reference Recency Set策略，模拟两条链表 s 和 q，s 中存储热数据块和 recency 小于 max(热数据recency) 的冷数据索引，q 中存储常驻冷数据块。
- insertion：当 cache miss 时，如果在 s 中也没有其索引时，insert 新块到 q 的头部，并将一个块索引添加到 s 的头部（不占 s 的容量）；如果在 s 中有其索引，insert 新块到 s 的头部，并将原来的索引删除。
- eviction：当 q full时，evict q尾部的若干块。
- promotion：当 hit 时，将新块从原位置删除，并提升到 s 的头部。
- demotion：当 s full时，将 s 尾部的若干热数据块删除并添加到 q 的头部，并且保证最后的 s 尾部是热数据块。
- idea：使用 recency 和 Inter-Reference（last reuse distance）作为预测前向重用距离的指标，q 中保存 |q| 个 recency 最小的冷数据，s 中保存 max(recency, Inter-Reference) 最小的 |s| 个热数据，并且 s 中还会保留 recency 小于 max(热数据recency) 的冷数据索引。
- 时间复杂度：O(n)

### DLIRS
Dynamic Low Inter-Reference Recency Set策略，结合 LIRS 和 ARC 策略。
- insertion：当 cache miss 时，如果在 s 中也没有其索引时，insert 新块到 q 的头部，并将一个块索引添加到 s 的头部（不占 s 的容量）；如果在 s 中有其索引，insert 新块到 s 的头部，并将原来的索引删除，同时也说明了 q 的容量少了，增加 q 的容量可能可以将这次 miss 变成 hit。
- eviction：当 q full时，evict q尾部的若干块。
- promotion：当 hit 时，将新块从原位置删除，并提升到 s 的头部。
- demotion：当 s full时，将 s 尾部的若干热数据块删除并添加到 q 的头部，并做标记，未来如果命中了这些块，说明 s 的容量少了，同时也需要注意保证最后的 s 尾部是热数据块。
- idea：同 LIRS 一样，使用 recency 和 Inter-Reference（last reuse distance）作为预测前向重用距离的指标，也从 ARC 中得到启发，动态调整 s 和 q 的容量。
- 时间复杂度：O(n)

### DIP
动态插入策略，在 LRU 和 LIP 策略中动态选择最优策略，不仅需要真实cache来模拟DIP策略，还需要额外的两倍空间来模拟只采用 LRU 和 LIP 策略时的缓存状态（在实际系统中只保存 key tag，不保存实际数据），当LRU策略发生 shadow cache 缺失时，psel计数器+1，当LIP策略发生 shadow cache 缺失时，psel计数器-1；对于DIP策略，如果psel计数器的最高有效位为0，则选择 LRU 作为真实cache的替换算法，而如果psel计数器的最高有效位为1，则选择 LIP 作为真实cache的替换算法。
- insertion：当 cache miss 时，如果选择的是 LRU，则 insert 新块到真实cache链表的头部，而如果选择的是 LIP，则 insert 新块到真实cache链表的尾部。
- eviction：当 insert 是如果 cache full，则 evict 链表尾部的若干块，直到 cache 有足够空间容纳新块。
- promotion：当 hit 时，将 hit块 从原位置提升到链表的头部。
- 时间复杂度：O(n)

### LFU（In-memory LFU）
最近最不经常使用策略，使用两个哈希表，一个存放 freq->list 的映射，便可以通过频率找到该频率下的若干 key；另一个存放 key->DlinkNode* 的映射，以便在 O(1) 的时间复杂度内对 hit块 进行promotion。
- insertion：当 cache miss 时 insert 新块到频率为1的链表的头部。
- eviction：当 insert 时如果 cache full，则 evict 频率最低的链表的尾部若干块，直到 cache 有足够空间容纳新块。
- promotion：当 hit 时，将 hit块 从原频率链表提升到 原频率+1 的链表的头部。
- idea：使用 fluency 作为预测前向重用距离的指标，认为 cache 中频率越低的块前向重用距离越大，越应该被驱逐，LFU有一个很大的问题就是 cache pollution，过去频率较高但以后再也不会被访问的块会一直占据缓存空间。
- 时间复杂度：O(n)

### MIN
最优策略，是一个离线算法，能够得到 trace 文件命中率上限。
- insertion：怎样效率高怎样来。
- eviction：当 insert 时如果 cache full，则 evict 前向重用距离最大的若干块，直到 cache 有足够空间容纳新块。
- promotion：怎样效率高怎样来。
- idea：直接使用前向重用距离指导 cache eviction，作为离线算法能够告诉我们一个 trace 的命中率优化上限。
- 时间复杂度：在这里有两个实现版本，一个为 O(mn)，一个为 O(nlogm)。

