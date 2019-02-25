# A simple B+ tree implemented in C

## Why I have this repository
I found that it was quite hard to find detailed tutorials or instructions explaining how to perform insertion and deletion on a B+ tree, so I implemented this B+ for people who what to learn this data structure.
网上的资料大部分都不全，找到的资料要么有代码没必要算法，要么有算法没有代码，要么就是演示一下怎么手动插入删除节点，而且这些演示并没有覆盖所有B+操作可能遇到的情况，所以我结合不同的资料实现了这个B+树，提供给想要学习这个数据结构的人们作参考

## Properties of this B+ tree
for any degree n:
1. all non-leaf nodes except root have [n / 2 + 1, n] children. e.g. n = 3, non-leaf nodes should have at least 2 children and 1 key
2. the number of keys a node has always equals the the number of children the node has minus 1. (num\_of\_keys = num\_of\_children - 1 
3. B+ tree is a sorted tree

B+树的标准不统一，实现各异，我的实现保证任何非根内部节点总是拥有至少n/2个和至多n-1个键，同时孩子的数量始终是键的数量+1.

For visualized example, you may try https://www.cs.usfca.edu/~galles/visualization/BPlusTree.html
But please notice that my B+ tree implementation is NOT the same as the implementation in the website above. Both of these implementations are valid. The difference is that the implementation in the website above allows internal nodes to have only one key and two children even if degree is greater than 3, and this implementation prefers redistribution when deleting a key while mine prefers merge. (you may check this)
上面那个网址是一个挺不错的B+树可视化工具，但是它的实现和我的不一样，它允许任何一个非根内部节点只有一个孩子，我的实现比这个严格一些。不过如果是一个3阶B+树，那么我的实现和这个网站的实现基本是一样的（删除的时候可能选择从不同的节点来借键或者合并，这个是小问题）

## Tips for reading this source code
- source files:
  - bplustree.c: implementation of a B+ tree
  - bplustree.h: interfaces of this B+ tree
  - list.c: a doubly-linked list
  - queue.c: linked queue

There are several functions to which you need to pay special attention: 
- insert: 
	- bpt\_complex\_insert: if insertion leads to separation of a node, call this function
	- bpt\_insert\_adjust: this function will perform complex adjustment to ensure the B+ tree is valid
- deletion:
    - redistribute\_leaf: if it is possible to borrow a key from a sibling to maintain the properties of B+ tree, call this function
	- redistribute\_internal: does what redistribute_leaf does to non-leaf nodes
	- merge\_leaves: merges a leaf into a chosen sibling
	- merge: merges non-leaf node into a chosen sibling
	- bpt\_complex_delete: if deletions leads the corruption of this B+ tree, call this function
    
着重关注bpt\_complex\_insert, bpt\_insert\_adjust，redistribute\_leaf, redistribute\_internal, merge\_leaves, merge这几个函数就行
## Algorithm to insert
insert is rather simple:
1. if there are enough space, just insert new data into the chosen leaf
2. if the leaf does not have enough space,
	1. insert data into this leaf
	2. halve this leaf
	3. generate a new node to be the parent of these two halves
3. insert this new parent into the parent of the leaf we halved in step 2

you may have noticed that step 3 leads to recursion
插入算法：
1. 叶节点空间总够就直接插入即可
2. 叶节点空间不够，依然把这个键插入（我的设计里面预留了一个位置来容纳额外的键），然后把这个节点拆分成两个，然后生成一个新的节点，以这个节点作为刚刚拆分出来的两个节点的父节点
3. 把这个新节点插入到刚才被我们拆分的那个节点的父节点中

显然这整个过程是递归的。
## Algorithm to delete
please refer to https://www.quora.com/What-is-the-algorithm-for-deletion-in-a-B+-tree
删除操作参考这个链接中的算法
While there are some hints I want to post here:
Algorithm above has described the procedure to delete a key in a B+ tree briefly, while some important details are not mentioned:
1. we need to locate an non-leaf node which contains the key to be deleted. If such node is located, replace the key in this node with the split key mentioned in algorithm above
2. it is possible that an internal node can not be merged into any sibling, we have to redistribute keys among this node, its chosen sibling and its parent. This is not mentions in algorithm above
3. if you are writing a B+ tree similar to mine in which keys are shared among parents and children (parent does not have any copy of any key), then you should be careful when deleting a key for it is likely for you to access freed memory

对这个算法我需要补充几点：
1. 在搜索删除键的过程中，我们要确定在树中有没有内部节点也包含这个将被删除的键，如果找到了这么一个键，我们得把这个节点中的这个键用我们找到的split key代替掉（不是删除掉）
2. 算法中没有提到内部节点也是有可能需要做redistribution的，在实现的时候要注意这一点
3. 如果你的实现和我的类似，每个键在树中只有叶节点保存着一份，其他内部节点只是将指针指向这个键，那么在删除的时候要小心一点，因为如果直接就把叶节点的空间free掉，在向上递归的时候，内部节点可能会访问到垃圾数据。
