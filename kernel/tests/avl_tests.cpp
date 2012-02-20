#include "AVLTree.h"
#include <stdio.h>
#include <stdlib.h>

void AVLTree::Print() const
{
	printf("\n");
	if (fDummyHead.right)
		Print(fDummyHead.right, 0, false);		
	printf("\n");
}

void AVLTree::Print(AVLNode *entry, int level, bool left) const
{
	for (int i = 0; i < level; i++)
		printf("   ");

	printf("   %s (%u - %u) max child depth %d\n", left ? "L" : "R", 
		entry->lowKey, entry->highKey, entry->maxChildDepth);
	if (entry->left) 
		Print(entry->left, level + 1, true);
	
	if (entry->right)
		Print(entry->right, level + 1, false);
}


void __assert_failed(int line, const char *file, const char *expr)
{
    printf("ASSERT FAILED (%s:%d): %s\n", file, line, expr);
    exit(1);
}

void __panic(int line, const char *file, const char *expr)
{
    printf("PANIC (%s:%d): %s\n", file, line, expr);
    exit(1);
}

   
const int numElements = 512; 

int main()
{
	AVLNode tstr[numElements];
	AVLTree tree;

	// Case #1: Linear insertion and deletion
	printf("Insert in order\n");
	for (int i = 0; i < numElements; i++)
		tree.AddElement(&tstr[i], i * 5, i * 5 + 4);

	printf("Find in order\n");
	for (int i = 0; i < numElements; i++)
		ASSERT(tree.FindElement(i * 5) == &tstr[i]);

	printf("Remove in order\n");
	for (int j = 0; j < numElements; j++) {
		tree.RemoveElement(&tstr[j]);
		for (int i = 0; i < numElements; i++) {
			if (i > j) {
				ASSERT(tree.FindElement(i * 5) == &tstr[i]); 
			} else {
				ASSERT(tree.FindElement(i * 5) == 0);
			}
		}
	}

	printf("Reverse\n");
	// Case #2: Reverse linear insertion and deletion
	for (int i = numElements - 1; i >= 0; i--)
		tree.AddElement(&tstr[i], i * 5, i * 5 + 4);

	for (int i = 0; i < numElements; i++)
		ASSERT(tree.FindElement(i * 5) == &tstr[i]);

	for (int j = numElements - 1; j >= 0; j--) {
		tree.RemoveElement(&tstr[j]);
		for (int i = 0; i < numElements; i++) {
			if (i < j) {
				ASSERT(tree.FindElement(i * 5) == &tstr[i]); 
			} else {
				ASSERT(tree.FindElement(i * 5) == 0);
			}
		}
	}

	printf("Overlaps\n");
	
	// Case #3: test for overlaps
	ASSERT(tree.IsRangeFree(15, 17) == true);
	ASSERT(tree.AddElement(&tstr[0], 10, 20) == &tstr[0]);
	ASSERT(tree.IsRangeFree(15, 17) == false);
	ASSERT(tree.AddElement(&tstr[1], 15, 17) == 0);
	ASSERT(tree.IsRangeFree(5, 25) == false);
	ASSERT(tree.AddElement(&tstr[1], 5, 25) == 0);
	ASSERT(tree.IsRangeFree(5, 17) == false);
	ASSERT(tree.AddElement(&tstr[1], 5, 17) == 0);
	ASSERT(tree.IsRangeFree(17, 25) == false);
	ASSERT(tree.AddElement(&tstr[1], 17, 25) == 0);
	tree.RemoveElement(&tstr[0]);	
	ASSERT(tree.IsRangeFree(15, 17) == true);


	// Case #4: Random insertion and deletion
	bool isInserted[numElements - 1];
	for (int i = 0; i < numElements; i++)
		isInserted[i] = false;

	for (int tries = 0; tries < 20; tries++) {
		// insert a bunch of stuff
		for (int i = 0; i < numElements; i++) {
			int j = rand() % numElements;
			while (isInserted[j])
				j = (j + 1) % numElements;

			isInserted[j] = true;
			tree.AddElement(&tstr[j], j * 5, j * 5 + 4);
			for (int k = 0; k < numElements; k++) {
				if (isInserted[k]) {
					ASSERT(tree.FindElement(k * 5) == &tstr[k]); 
				} else {
					ASSERT(tree.FindElement(k * 5) == 0);
				}
			}
		}
	
		tree.Print();

		for (int i = 0; i < numElements - 1; i++)  
			ASSERT(tree.NextElement(&tstr[i]) == &tstr[i + 1]);	

		ASSERT(tree.NextElement(&tstr[numElements - 1]) == 0);
		for (int i = 0; i < numElements - 1; i++)  
			ASSERT(tree.PreviousElement(&tstr[i + 1]) == &tstr[i]);	

		ASSERT(tree.PreviousElement(&tstr[0]) == 0);

		
		int i = numElements - 1;
		for (AVLTreeIterator iterator(&tree, false); iterator.CurrentElement();
			iterator.GoToNext()) {
			ASSERT(tree.CurrentElement() == &tstr[i--]);
		}

		i = 0;
		for (AVLTreeIterator iterator(&tree, true); iterator.CurrentElement();
			iterator.GoToNext()) {
			ASSERT(tree.CurrentElement() == &tstr[i++]);
		}

		// delete a bunch of stuff
		for (int i = 0; i < numElements; i++) {
			int j = rand() % numElements;
			while (!isInserted[j])
				j = (j + 1) % numElements;

			isInserted[j] = false;
			tree.RemoveElement(&tstr[j]);
			for (int k = 0; k < numElements; k++) {
				if (isInserted[k]) {
					ASSERT(tree.FindElement(k * 5) == &tstr[k]); 
				} else {
					ASSERT(tree.FindElement(k * 5) == 0);
				}
			}
		}

		printf("Ok...\n");
	}

	printf("Tree tests passed\n");
}