#ifndef _AVL_TREE_H
#define _AVL_TREE_H

const int kMaxTreeHeight = 32;

class AVLNode {
public:
	AVLNode();
	virtual ~AVLNode() {}
	inline unsigned int GetLowKey() const;
	inline unsigned int GetHighKey() const;

private:
	inline void RecomputeMaxChildDepth();
	inline int GetBalance() const;
    inline void RotateLeft(AVLNode *parent);
	inline void RotateRight(AVLNode *parent);

	unsigned int fLowKey;
	unsigned int fHighKey;	
	AVLNode *fLeft;
	AVLNode *fRight;
	unsigned int fMaxChildDepth;

	friend class AVLTree;
	friend class AVLTreeIterator;
};

class AVLTree {
public:
	AVLNode* Add(AVLNode*, unsigned int lowKey, unsigned int highKey);
	AVLNode* Remove(AVLNode*);	
	AVLNode* Find(unsigned int key) const;
	AVLNode* Resize(AVLNode*, unsigned int newSize);
	bool IsRangeFree(unsigned int lowKey, unsigned int highKey) const;

private:
	static void FixBalance(AVLNode *stack[], int);
	
	// The real head of the tree is the right child of this
	// dummy node.  Using a dummy node eliminates a bunch of
	// special cases in the rotation code.
	AVLNode fDummyHead;

	friend class AVLTreeIterator;
};

class AVLTreeIterator {
public:
	AVLTreeIterator(const AVLTree&, bool forward);
	void GoToNext();
	inline AVLNode *GetCurrent() const;

private:
	bool fIteratingForward;
	AVLNode *fCurrentNode;
	int fStackDepth;
	AVLNode *fStack[kMaxTreeHeight];
};

inline unsigned int AVLNode::GetLowKey() const
{
	return fLowKey;
}

inline unsigned int AVLNode::GetHighKey() const
{
	return fHighKey;
}

inline AVLNode* AVLTreeIterator::GetCurrent() const
{
	return fCurrentNode;
}

#endif
