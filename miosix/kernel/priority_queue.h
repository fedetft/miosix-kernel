/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/*
 * Theory of operation:
 * Finding the first free element in a complete binary tree is done using
 * path and delimiter variables. path contains the number of elements in the
 * heap plus one, but its bits implicitly represent the path to the first free
 * node. The first bit set in path (MSB) is called the delimiter, and must be
 * discarded from the path. All bits from (1<<delimiter-1) to (1<<0) are the
 * path. Zero means left, 1 means right.
 * Example:
 *       9
 *     /   \
 *    8     6
 *   / \   /
 *  1   4 X
 * The first free element is X, its path is right, left.
 * The number of nodes is 5, plus one 6, that in binary is 110. Discarding the
 * MSB, the path is 1=right, 0=left. This is how path works. The other variable,
 * delimiter is used to "point" to the delimiter bit. (1<<delimiter) points
 * to the delimiter bit in path. Every time an element is added/removed,
 * if the MSB moves, delimiter must be updated.
 * The base case is the empty tree, represented with path = 1 and delimiter = 0.
 * The only set bit in path is the delimiter.
 */

#ifndef PRIORITY_QUEUE_H
#define	PRIORITY_QUEUE_H

#include <algorithm> //For std::swap
#include <functional> //For std::less
//#include <cassert>

namespace miosix {

/**
 * \internal
 * This class is meant to be a class nearly compatible with the STL's
 * std::priority_queue. The reason why this class is used instead of the STL one
 * is for code size issues.
 * User code should not use this class since it might be modified/removed in
 * the future.
 * \param T type of object stored in the queue
 * \param Compare comparison functor, to compare two T objects
 */
template<typename T, class Compare = std::less<T> >
class PriorityQueue
{
public:
    
    /**
     * Constructor, builds an empty queue
     */
    PriorityQueue() : root(new Node), path(1), delimiter(0) {}

    /**
     * \return true if the queue is empty
     */
    bool empty() const
    {
        return path==1;
    }

    /**
     * \return the number of elements in the queue
     */
    unsigned int size() const
    {
        return path-1;
    }

    /**
     * \return a const reference to the topmost element in the queue
     */
    const T& top() const
    {
        return root->data;
    }

    /**
     * Add an element to the queue
     * \param x element to add
     *
     * Function is not inle because it is not trivial.
     */
    void push(const T& x);

    /**
     * Remove the topmost element from the queue
     * Function is not inle because it is not trivial.
     */
    void pop();

    /**
     * Destructor, deletes the heap and its content
     */
    ~PriorityQueue()
    {
        delete root;
    }

private:

    /**
     * A node of the heap
     */
    class Node
    {
    public:
        T data;
        Node *left;
        Node *right;

        Node() : data(), left(0), right(0) {}
        Node(const T& x) : data(x), left(0), right(0) {}

        /**
         * Not inline since recursive inline destructors cause code duplication.
         */
        ~Node();
    private:
        Node(const Node& n);
        Node & operator=(Node& n);
    };

    /**
     * Perform the sift up operation on a heap if child node is bigger/smaller
     * than parent node swap them
     * \param parent parent node
     * \param child child node
     */
    void siftUp(Node *parent, Node *child)
    {
        using std::swap;
        if(compfn(child->data,parent->data)) swap(child->data,parent->data);
    }

    /**
     * Add an element to the heap by recursively descending the tree, adding it
     * at the first free place at the bottom, and sifting up if required.
     * \param n
     * \param x
     * \param delimiter
     *
     * Not inline because it is not trivial.
     */
    void recursiveAdd(Node *n, const T& x, unsigned char delimiter);

    Node *root; ///<Root of the tree
    ///Bits of this variable represent the path to the first empty node of the
    ///tree exept the first set bit. 0=left 1=right
    unsigned int path;
    unsigned char delimiter; ///<Points to the delimiter (first set bit in path)
    Compare compfn; ///<Compare functor
};

template<typename T, class Compare>
void PriorityQueue<T,Compare>::push(const T& x)
{
    if(this->empty())
    {
        root->data=x;
        path=2; //path=10b
        delimiter=1; //(1<<delimiter) points to the delimiter bit in path
    } else {
        recursiveAdd(root,x,delimiter-1);
        path++;
        if((path & (1<<delimiter))==0) delimiter++;
    }
}

template<typename T, class Compare>
void PriorityQueue<T,Compare>::pop()
{
    using std::swap;
    if(this->empty()) return;
    path--;
    if((path & (1<<delimiter))==0) delimiter--;
    if(this->empty()) //There was only one node
    {
        //assert(root->left==0);
        //assert(root->right==0);
        delete root;
        root=new Node();
        return;
    }
    //There was more than one node, walk till the last
    Node *walk=root;
    for(unsigned char i=delimiter-1;i>=1;i--)
    {
        if(path & (1<<i)) walk=walk->right;
        else walk=walk->left;
    }
    //Delete root and swap last element with root
    if(path & (1<<0))
    {
        //assert(walk->right->left==0);
        //assert(walk->right->right==0);
        swap(root->data,walk->right->data);
        delete walk->right;
        walk->right=0;
    } else {
        //assert(walk->left->left==0);
        //assert(walk->left->right==0);
        swap(root->data,walk->left->data);
        delete walk->left;
        walk->left=0;
    }
    //Sift down
    walk=root;
    for(;;)
    {
        bool leftOk=walk->left==0 || compfn(walk->data,walk->left->data);
        bool rightOk=walk->right==0 || compfn(walk->data,walk->right->data);
        if(leftOk && rightOk) break; //No need to sift down
        if(!leftOk && !rightOk)
        {
            //Heap property is violated by both childs, swap with the
            //bigger/smaller (depending on max-heap or min-heap)
            if(compfn(walk->left->data,walk->right->data))
            {
                swap(walk->data,walk->left->data);
                walk=walk->left;
            } else {
                swap(walk->data,walk->right->data);
                walk=walk->right;
            }
        } else if(!leftOk) {
            //Heap property violated by left node
            swap(walk->data,walk->left->data);
            walk=walk->left;
        } else {
            //Heap property violated by right node
            swap(walk->data,walk->right->data);
            walk=walk->right;
        }
    }
}

template<typename T, class Compare>
PriorityQueue<T,Compare>::Node::~Node()
{
    if(left!=0) delete left;
    if(right!=0) delete right;
}

template<typename T, class Compare>
void PriorityQueue<T,Compare>::recursiveAdd(Node *n, const T& x,
    unsigned char delimiter)
{
    if(delimiter==0)
    {
        //Add element
        if(path & (1<<0))
        {
            //assert(n->right==0);
            n->right=new Node(x);
            siftUp(n,n->right);
        } else {
            //assert(n->left==0);
            n->left=new Node(x);
            siftUp(n,n->left);
        }
    } else {
        //Recursively go down the tree
        if((path & (1<<delimiter)))
        {
            recursiveAdd(n->right,x,delimiter-1);
            siftUp(n,n->right);
        } else {
            recursiveAdd(n->left,x,delimiter-1);
            siftUp(n,n->left);
        }
    }
}

} //namespace miosix

#endif	//PRIORITY_QUEUE_H
