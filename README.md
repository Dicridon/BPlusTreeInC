# A simple B+ tree implemented in C

## Why I have this repository
I found that it was quite hard to find detailed tutorials or instructions explaining how to perform insertion and deletion on a B+ tree, so I implemented this B+ for people who what to learn this data structure.

## Properties of this B+ tree
for any degree n:
1. all non-leaf nodes except root have [n / 2 + 1, n] children. e.g. n = 3, non-leaf nodes should have at least 2 children and 1 key
2. the number of keys a node has always equals the the number of children the node has minus 1. (num\_of\_keys = num\_of\_children - 1 
3. B+ tree is a sorted tree

For visualized example, you may try https://www.cs.usfca.edu/~galles/visualization/BPlusTree.html
But please notice that my B+ tree implementation is NOT the same as the implementation in the website above. Both of these implementations are valid. The difference is that the implementation in the website above allows internal nodes to have only one key and two children even if degree is greater than 3, and this implementation prefers redistribution when deleting a key while mine prefers merge. (you may check this)

## Tips for reading this source code
- source files:
  - bplustree.c: implementation of a B+ tree
  - bplustree.h: interfaces of this B+ tree
  - list.c: a doubly-linked list
  - queue.c: linked queue

There are several functions to which you need to pay special attention: 
- insert: 
	- bpt\_complex\_insert: if insertion leads to seperation of a node, call this function
	- bpt\_insert\_adjust: this function will perform complex adjustment to ensure the B+ tree is valid
- deletion:
    - redistribute\_leaf: if it is possible to borrow a key from a sibling to maintain the properties of B+ tree, call this function
	- redistribute\_internal: does what redistribute_leaf does to non-leaf nodes
	- merge\_leaves: merges a leaf into a chosen sibling
	- merge: merges non-leaf node into a chosen sibling
	- bpt\_complex_delete: if deletions leads the corruption of this B+ tree, call this fuinction

## Algorithm to insert
insert is rather simple:
1. if there are enough space, just insert new data into the chosen leaf
2. if the leaf does not have enough sapce,
	1. insert data into this leaf
	2. halve this leaf
	3. generate a new node to be the parent of these two halves
3. insert this new parent into the parent of the leaf we havled in step 2

you may have noticed that step 3 leads to recursion

## Algorithm to delete
please refer to https://www.quora.com/What-is-the-algorithm-for-deletion-in-a-B+-tree

While there are some hints I want to post here:
Algorithm above has described the procudre to delete a key in a B+ tree briefly, while some important details are not memtinoed:
1. we need to locate an non-leaf node which contains the key to be deleted. If such node is located, replace the key in this node with the split key mentioned in algorithm above
2. it is possible that an internal node can not be merged into any sibling, we have to redistribute keys among this node, its chosen sibling and its parent. This is not mentions in algorithm above
3. if you are writing a B+ tree similar to mine in which keys are shared among parents and children (parent does not have any copy of any key), then you should be careful when deleting a key for it is likely for you to access freed memory