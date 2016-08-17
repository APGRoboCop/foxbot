//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//	Doubly Linked List Data Structure
//
// (http://foxbot.net)
//
// list.h
//
// Copyright (C) 2003 - Tom "Redfox" Simpson
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details at:
// http://www.gnu.org/copyleft/gpl.html
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

// forward reference to the LIter class
template <typename Type> class LIter;

template <typename Type> class List
{
    // allow LIter to be a friend class
    friend class LIter<Type>;

protected:
    struct Node {
        Type element;      // one cell of data to store
        Node *prev, *next; // previous and next elements
    };

    // The total number of elements.
    int total;

    // Access to the left and right ends of the list.
    Node *head, *tail;

public:
    // ctr : Initialize the list to contain 0 things.
    //
    List()
    {
        head = tail = NULL;
        total = 0;
    }

    // dtr : Clear all list information
    //
    ~List()
    {
        clear();
        total = 0;
    }

    // algorithms
    ////////////////////

    // isEmpty : test if list is empty
    bool isEmpty()
    {
        return (head == NULL);
    }

    // add : insert a new node to the beginning / end
    void addHead(const Type& v);
    void addTail(const Type& v);

    // insert / remove : place or remove an item
    //                   in the list at given position
    void insert(const Type& v, LIter<Type>& loc);
    void remove(LIter<Type>& loc);

    // clear : remove all nodes from the list
    void clear();

    // size : return the number of elements we have.
    int size();
};

// addHead : create a new item and attach it to
//           the list at the head
//
template <typename Type> void List<Type>::addHead(const Type& v)
{
    // create node and store the value in it.
    Node* n = new Node;
    n->element = v;
    // set prev to NULL, since it will be the new head.
    n->prev = NULL;

    // attach to the head, and update the head ptr.
    n->next = head;
    if(head != NULL)
        head->prev = n;
    head = n;

    total++;

    // special case : set tail pointer to n if node is only one
    //                in the list.
    if(tail == NULL)
        tail = n;
}

// addTail : create a new item and attach it to
//           the list at the tail
//
template <typename Type> void List<Type>::addTail(const Type& v)
{
    // create node and store the value in it.
    Node* n = new Node;
    n->element = v;
    // set next to NULL, since it will be the new tail.
    n->next = NULL;

    // attach to the end of tail (if there is one)
    // and update the tail ptr.
    if(tail != NULL)
        tail->next = n;
    n->prev = tail;
    tail = n;

    total++;

    // special case : set head pointer to n if node is only one
    //                in the list.
    if(head == NULL)
        head = n;
}

// clear : delete all nodes and empty the list
//
template <typename Type> void List<Type>::clear()
{
    // delete head node over and over again
    // until list is empty
    while(!isEmpty()) {
        Node* n = head;
        head = head->next;
        delete n;
    }

    total = 0;

    // set tail to NULL, since it doesn't get
    // changed via the code above.
    tail = NULL;
}

template <typename Type> int List<Type>::size()
{
    return total;
}

// LIter class defines an iterator for the List class.
//
template <typename Type> class LIter
{
    // allow List to be a friend class
    friend class List<Type>;

private:
    // Keep track of the list this iterator is working with
    List<Type>* list;
    // Also keep track of the current node.

    //	  template <typename Type>
    typename List<Type>::Node* currentNode;

public:
    // ctr : initialize the iterator class by specifying the list
    //       it will be used with.
    LIter(List<Type>* listToUse)
    {
        list = listToUse;
    }

    // begin : start the iterator at the head or tail
    //         of the list to start an iteration.
    //
    void begin()
    {
        currentNode = list->head;
    }
    void beginReverse()
    {
        currentNode = list->tail;
    }

    // end : test to see if the iterator has finished going
    //       through all of the nodes
    bool end()
    {
        return (currentNode == NULL);
    }

    // ++operator : increment the current node.
    //
    // Note : prefix use only (++iter).  Postfix not supported.
    LIter<Type>& operator++()
    {
        if(currentNode != NULL)
            currentNode = currentNode->next;
        return *this;
    }

    // --operator : decrement the current node.
    //
    // Note : prefix use only (--iter).  Postfix not supported.
    LIter<Type>& operator--()
    {
        if(currentNode != NULL)
            currentNode = currentNode->prev;
        return *this;
    }

    // current : return the address of the current item.
    //
    Type* current()
    {
        if(currentNode == NULL)
            return NULL;
        return &(currentNode->element);
    }
};

// insert : place an item at the given iterator pos.
//
// Note : after insert, iterator will be positioned
//        on top of the node following the new node.
template <typename Type> void List<Type>::insert(const Type& v, LIter<Type>& loc)
{
    // store a tmp. ptr to node in the location where
    // insert will occur.
    Node* nextNode = loc.currentNode;
    // inserting on a NULL location (beyond ends of the list)
    // is invalid, simply return -> addHead or addTail should be called.
    if(nextNode == NULL)
        return;

    // create the new node and fill it.
    Node* newNode = new Node;
    newNode->element = v;

    // get a ptr to the node before the current.  It will
    // end up being the prev node to the new one.
    Node* prevNode = nextNode->prev;

    // attach the new node into the list.
    // connections between prevNode and newNode
    if(prevNode != NULL)
        prevNode->next = newNode;
    newNode->prev = prevNode;
    // connections between newNode and nextNode
    newNode->next = nextNode;
    nextNode->prev = newNode;

    total++;

    // special case : handle adding to the beginning of the
    //                list.  If prevNode is NULL, newNode is the head.
    if(prevNode == NULL)
        head = newNode;

    // fix the iterator to make it point to the new location
    loc.currentNode = newNode->next;
}

// remove : remove an item at the given iterator pos.
//
// Note : after remove, iterator will be positioned
//        on the node which directly followed the given
//        node (or NULL if the node was the tail)
template <typename Type> void List<Type>::remove(LIter<Type>& loc)
{
    // store a tmp. ptr to node in the location where
    // remove will occur.
    Node* deletedNode = loc.currentNode;

    // attach nodes to the left and right of this node
    // to each other.  Also note that there may not
    // be nodes to the right and left.
    Node* leftNode = deletedNode->prev;
    Node* rightNode = deletedNode->next;
    if(leftNode != NULL)
        leftNode->next = rightNode;
    if(rightNode != NULL)
        rightNode->prev = leftNode;

    // special cases : we are possibly deleting
    // the head and/or tail of the list.  Handle
    // these cases by updating those pointers if necessary.
    if(deletedNode == head)
        head = deletedNode->next;
    if(deletedNode == tail)
        tail = deletedNode->prev;

    total--;

    // update the iterator to the node after this one.
    loc.currentNode = deletedNode->next;

    // delete the memory that this node was taking up
    delete deletedNode;
}
