
struct Node {
    int value = 0;
    Node *prev = nullptr;
    Node *next = nullptr;
};

class DoublyLinkedList {
    public:
        Node *head = nullptr;
        Node *tail = nullptr;
        int length = 0;

        DoublyLinkedList() {}

        ~DoublyLinkedList() {
            Node* head_ptr = head;
            Node* next_ptr;
            while (head_ptr != nullptr) {
                next_ptr = head_ptr->next;
                delete head_ptr;
                head_ptr = next_ptr;
            }
            head = nullptr;
            tail = nullptr;
        }

        void append(Node *node) {
            if (tail == nullptr) {
                tail = node;
                head = node;
            } else {
                tail->next = node;
                node->prev = tail;
                tail = node;
            }
            length++;
        }

        void append(int value) {
            Node *node = new Node();
            node->value = value;
            append(node);
        }

        void append_left(Node *node) {
            if (tail == nullptr) {
                head = node;
                tail = node;
            } else {
                head->prev = node;
                node->next = head;
                head = node;
            }
            length++;
        }

        void append_left(int value) {
            Node *node = new Node();
            node->value = value;
            append_left(node);
        }

        Node* pop() {
            Node *res = nullptr;
            if (tail != nullptr) {
                res = tail;
                if (length >= 2) {
                    res->prev->next = nullptr;
                    tail = res->prev;
                    res->prev = nullptr;
                } else {
                    head = nullptr;
                    tail = nullptr;
                }
            }
            length -= 1;
            return res;
        }

        Node* pop_left() {
            Node* res = nullptr;
            if (head != nullptr) {
                res = head;
                if (length >= 2) {
                    res->next->prev = nullptr;
                    head = head->next;
                    head->next = nullptr;
                } else {
                    head = nullptr;
                    tail = nullptr;
                }
            }
            length -= 1;
            return res;
        }
};