// 
// Copyright 1998-2012 Jeff Bush
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 

#include "AVLTree.h"
#include "KernelDebug.h"

AVLNode::AVLNode()
	:	fLowKey(0),
		fHighKey(0),
		fLeft(0),
		fRight(0),
		fMaxChildDepth(0)
{
}

inline void AVLNode::RecomputeMaxChildDepth()
{
	if (fLeft != 0 && fRight == 0)
		fMaxChildDepth = fLeft->fMaxChildDepth + 1;
	else if (fRight != 0 && fLeft == 0)
		fMaxChildDepth = fRight->fMaxChildDepth + 1;
	else if (fRight == 0 && fLeft == 0)
		fMaxChildDepth =  0;
	else 
		fMaxChildDepth = (fRight->fMaxChildDepth > fLeft->fMaxChildDepth ?
			fRight->fMaxChildDepth : fLeft->fMaxChildDepth) + 1;
}

inline int AVLNode::GetBalance() const
{
	if (fLeft == 0 && fRight == 0) 
		return 0;
	else if (fLeft == 0)
		return fRight->fMaxChildDepth + 1;
	else if (fRight == 0)
		return -(fLeft->fMaxChildDepth + 1);
	else
		return fRight->fMaxChildDepth - fLeft->fMaxChildDepth;	
}

inline void AVLNode::RotateLeft(AVLNode *parent)
{
	AVLNode *child = fRight;
	fRight = child->fLeft;
	child->fLeft = this;
	if (parent->fLeft == this)
		parent->fLeft = child;
	else
		parent->fRight = child;

	RecomputeMaxChildDepth();
	child->RecomputeMaxChildDepth();
}

inline void AVLNode::RotateRight(AVLNode *parent)
{
	AVLNode *child = fLeft;
	fLeft = child->fRight;
	child->fRight = this;
	if (parent->fLeft == this)
		parent->fLeft = child;
	else
		parent->fRight = child;

	RecomputeMaxChildDepth();
	child->RecomputeMaxChildDepth();
}

AVLNode* AVLTree::Add(AVLNode *node, unsigned int lowKey, unsigned int highKey)
{
	bool needsBalanceFixed = false;
	AVLNode *stack[kMaxTreeHeight];
	int depth = 0;
	AVLNode *parent = &fDummyHead;
	for (;;) {
		ASSERT(depth < kMaxTreeHeight);
		stack[depth++] = parent;
		if (highKey < parent->fLowKey) {
			if (parent->fLeft)
				parent = parent->fLeft;
			else {
				parent->fLeft = node;
				// If this node didn't have a child before, then the height
				// along this path has changed.
				if (parent->fRight == 0)
					needsBalanceFixed = true;

				break;
			}
		} else if (lowKey > parent->fHighKey || parent->fHighKey == 0) {
			if (parent->fRight)
				parent = parent->fRight;
			else {
				parent->fRight = node;
				if (parent->fLeft == 0)
					needsBalanceFixed = true;

				break;
			}
		} else
			return 0;
	}

	// This needs to be filled in before rebalancing.
	node->fLowKey = lowKey;
	node->fHighKey = highKey;
	node->fRight = 0;
	node->fLeft = 0;
	node->fMaxChildDepth = 0;

	if (needsBalanceFixed) 
		FixBalance(stack, depth);

	return node;
}

AVLNode* AVLTree::Remove(AVLNode *node)
{
	AVLNode *stack[kMaxTreeHeight];
	int depth = 0;

	// Search for the node.
	AVLNode *parent = &fDummyHead;
	while (parent) {
		stack[depth++] = parent;
		ASSERT(depth < kMaxTreeHeight);
		if (node->fLowKey < parent->fLowKey) {
			if (parent->fLeft == node)
				break;
			else
				parent = parent->fLeft;
		} else if (node->fHighKey > parent->fHighKey) {
			if (parent->fRight == node)
				break;
			else
				parent = parent->fRight;
		}
	}

	if (parent == 0)
		return 0;	// Node was not in the tree.
	
	AVLNode *successor = 0;
	if (node->fRight == 0) {
		// Case 1: Dead node has no right child, replace the
		// dead node with its left child.
		successor = node->fLeft;
	} else if (node->fRight->fLeft == 0) {
		// Case 2: Dead node has a right child, but that child lacks
		// a left child.  Make the dead node's left child a child
		// of the dead node's right child.
		successor = node->fRight;
		successor->fLeft = node->fLeft;
	} else {
		// Case 3: Dead node has left and right children.
		// Start at right child and iterate left as far as
		// possible (ie there are no nodes that are between
		// this one and the leftmost node in sequence).
		// Give the leftmost nodes right child to its parent
		// and replace the current node with it.
		AVLNode *leftmostParent = node->fRight;
		int newStackEntry = depth++;
		while (leftmostParent->fLeft->fLeft) {
			stack[depth++] = leftmostParent;
			ASSERT(depth < kMaxTreeHeight);
			leftmostParent = leftmostParent->fLeft;
		}

		successor = leftmostParent->fLeft;

		// Free leftmost node by giving its right child to its parent.
		leftmostParent->fLeft = successor->fRight;
		successor->fLeft = node->fLeft;
		successor->fRight = node->fRight;

		// Stuff this in the stack so the whole path will get
		// rebalanced (the balance was changed by removing the leaf)
		// Store this in spot on the stack that was saved earlier (in
		// newStackEntry), and stick it back there so the stack will
		// still be in order.
		stack[newStackEntry] = successor;
	}

	if (parent->fLeft == node)
		parent->fLeft = successor;
	else
		parent->fRight = successor;

	FixBalance(stack, depth);
	return node;
}

AVLNode* AVLTree::Find(unsigned int key) const
{
	AVLNode *node = fDummyHead.fRight;
	while (node) {
		if (key < node->fLowKey)
			node = node->fLeft;
		else if (key > node->fHighKey)
			node = node->fRight;
		else 
			break;	
	}
	
	return node;
}

AVLNode* AVLTree::Resize(AVLNode *node, unsigned int newSize)
{
	node->fHighKey = node->fLowKey + newSize;
}

bool AVLTree::IsRangeFree(unsigned int lowKey, unsigned int highKey) const
{
	AVLNode *node = fDummyHead.fRight;
	while (node) {
		if (highKey < node->fLowKey)
			node = node->fLeft;
		else if (lowKey > node->fHighKey)
			node = node->fRight;
		else
			return false;
	}

	return true;
}

void AVLTree::FixBalance(AVLNode *stack[], int stackDepth)
{
	// Don't check balance at depth 0, as it is a dummy node.  The real
	// root is always its right child.
	for (int depth = stackDepth - 1; depth > 0; depth--) {
		AVLNode *current = stack[depth];
		current->RecomputeMaxChildDepth();
		if (current->GetBalance() > 1) {
			// Imbalanced to the right
			current->fRight->RecomputeMaxChildDepth();
			if (current->fRight->GetBalance() < 0) 	
				current->fRight->RotateRight(current);	

			current->RotateLeft(stack[depth - 1]);
		} else if (current->GetBalance() < -1) {
			// Imbalanced to the left
			current->fLeft->RecomputeMaxChildDepth();
			if (current->fLeft->GetBalance() > 0) 
				current->fLeft->RotateLeft(current);		
				
			current->RotateRight(stack[depth - 1]);
		}
	}
}

AVLTreeIterator::AVLTreeIterator(const AVLTree &tree, bool forward)
	:	fIteratingForward(forward),
		fCurrentNode(tree.fDummyHead.fRight),
		fStackDepth(0)
{
	if (fCurrentNode == 0) 
		return ;

	if (fIteratingForward) {
		while (fCurrentNode->fLeft) {
			fStack[fStackDepth++] = fCurrentNode;
			fCurrentNode = fCurrentNode->fLeft;
		}
	} else {
		while (fCurrentNode->fRight) {
			fStack[fStackDepth++] = fCurrentNode;
			fCurrentNode = fCurrentNode->fRight;
		}
	}
}

void AVLTreeIterator::GoToNext()
{
	if (fIteratingForward) {
		if (fCurrentNode->fRight) {
			fCurrentNode = fCurrentNode->fRight;
			while (fCurrentNode->fLeft) {
				fStack[fStackDepth++] = fCurrentNode;
				fCurrentNode = fCurrentNode->fLeft;
			}
		} else {
			if (fStackDepth < 1) 
				fCurrentNode = 0;		// End
			else 
				fCurrentNode = fStack[--fStackDepth];	
		}
	} else {
		if (fCurrentNode->fLeft) {
			fCurrentNode = fCurrentNode->fLeft;
			while (fCurrentNode->fRight) {
				fStack[fStackDepth++] = fCurrentNode;
				fCurrentNode = fCurrentNode->fRight;
			}
		} else {
			if (fStackDepth < 1) 
				fCurrentNode = 0;		// End
			else 
				fCurrentNode = fStack[--fStackDepth];	
		}
	}
}
