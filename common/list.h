#ifndef LIST_H
#define LIST_H

template <typename NodeTy, typename ParentTy> struct ListWithParent;

template <typename NodeTy, typename ParentTy>
struct ListNode {
  using ListTy = ListWithParent<NodeTy, ParentTy>;
  using ListNodeTy = ListNode<NodeTy, ParentTy>;
  ListNodeTy *prev;
  ListNodeTy *next;

  /// Forward to NodeTy::get_parent().
  ///
  /// Note: do not use the name "get_parent()".  We want a compile error
  /// (instead of recursion) when the subclass fails to implement \a
  /// getParent().
  ParentTy *get_node_parent() {
    return static_cast<NodeTy *>(this)->get_parent();
  }

  // Anything that you insert should be heap-allocated!
  // Insert `new_node` after the current one.
  void insert_after(NodeTy *new_node) {
    // Make sure it has a parent, otherwise
    // we consider it to be unlinked.
    ParentTy *parent = get_node_parent();
    assert(parent && "Node does not have a parent; it's unlinked.");

    new_node->next = this->next;
    new_node->prev = this;
    if (this->next) {
      this->next->prev = new_node;
    }
    this->next = new_node;

    // Make sure that the two parents are the same
    ParentTy *new_node_parent = new_node->get_node_parent();
    assert(parent == new_node_parent && "The two nodes have a different parent.");
    // Update the parent
    ListTy *sublist = parent->get_sublist();
    sublist->inserted_after(this);
  }

  // Unlink this node from the list.
  void unlink() {
    // Make sure it has a parent, otherwise
    // we consider it to already be unlinked.
    ParentTy *parent = get_node_parent();
    assert(parent && "The node does not have a parent; it's already unlinked.");

    if (this->prev) {
      this->prev->next = this->next;
    }
    if (this->next) {
      this->next->prev = this->prev;
    }
    // Update the parent
    ListTy *sublist = parent->get_sublist();
    sublist->unlinked(this);
  }

  ListNode() {
    prev = next = nullptr;
  }
};

template <typename NodeTy, typename ParentTy>
struct ListWithParent {
  using ListNodeTy = ListNode<NodeTy, ParentTy>;
  ListNodeTy *head, *tail;
  size_t size;

  ListWithParent() {
    head = tail = nullptr;
  }

  // Anything that you insert should be heap-allocated!
  void insert_at_end(NodeTy *new_node) {
    new_node->next = nullptr;
    if (!head) {
      assert(!tail);
      head = tail = new_node;
      new_node->prev = nullptr;
    } else {
      assert(tail);
      tail->next = new_node;
      new_node->prev = tail;
      tail = new_node;
    }
    size++;
  }

  void inserted_after(ListNodeTy *n) {
    if (n == tail) {
      tail = n;
    }
    size++;
  }

  void unlinked(ListNodeTy *n) {
    // Note: This also works when `n` is _both_
    // the `head` and the `tail`.
    if (n == head) {
      head = n->next;
    }
    if (n == tail) {
      tail = n->prev;
    }
    size--;
  }

  size_t num_nodes() const {
    return size;
  }

  struct Iterator {
    Iterator(ListNodeTy *n) {
      this->curr = n;
    }

    Iterator& operator=(ListNodeTy *n) {
      this->curr = n;
      return *this;
    }

    // Prefix ++
    Iterator& operator++() {
      if (curr)
        curr = curr->next;
      return *this;
    }

    // Postfix ++
    Iterator operator++(int) {
      Iterator it = *this;
      ++*this;
      return it;
    }

    bool operator!=(const Iterator& it) {
      return curr != it.curr;
    }

    // IMPORTANT: You get back a _pointer_ to NodeTy
    NodeTy *operator*() {
      return (NodeTy *) curr;
    }

    ListNodeTy *curr;
  };

  Iterator begin() const { return head; }
  Iterator end() const { return nullptr; }
};

#endif
