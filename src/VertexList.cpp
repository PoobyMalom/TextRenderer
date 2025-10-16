#include "VertexList.h"

Vertex* createVertexList(const std::vector<int16_t> xCoordinates, const std::vector<int16_t> yCoordinates, const std::vector<uint8_t> flags) {
  if (xCoordinates.size() != yCoordinates.size() || xCoordinates.empty()) {
    return nullptr;
  }
  
  Vertex* head = nullptr;
  Vertex* tail = nullptr;
  
  for (size_t i = 0; i < xCoordinates.size(); ++i) {
    Vertex* v = new Vertex(xCoordinates[i], yCoordinates[i], flags[i]);
    
    if (!head) {
      head = tail = v;
    } else {
      tail->next = v;
      v->prev = tail;
      tail = v;
    }
  }
  
  // Close the circular list
  if (head && tail) {
    tail->next = head;
    head->prev = tail;
  }
  
  return head;
}

// Remove a node from its ring (does NOT delete it). Returns the next node.
Vertex* detachNode(Vertex* n) {
  if (!n) return nullptr;
  Vertex* nxt = n->next;
  Vertex* prv = n->prev;
  if (nxt && prv) {
    prv->next = nxt;
    nxt->prev = prv;
  }
  // isolate n
  n->next = n->prev = nullptr;
  return nxt;
}

void deleteVertexList(Vertex* head) {
  if (!head) return;
  Vertex* cur = head->next;
  while (cur && cur != head) {
    Vertex* tmp = cur;
    cur = cur->next;
    delete tmp;
  }
  delete head;
}