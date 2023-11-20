struct Order final {
    const bool is_bid;
    int quantity;
    const int price;
    const int id;
    Order *prev;
    Order *next;
    Order (const bool _is_bid, int _quantity, const int _price, const int _id) 
        : is_bid(_is_bid), quantity(_quantity), price(_price), id(_id), prev(nullptr), next(nullptr) {}
};

class DoublyLinkedList {
    public:
        Order *head;
        Order *tail;
        int length = 0;

        DoublyLinkedList(): tail(nullptr), head(nullptr), length(0) {}

        ~DoublyLinkedList() {
            Order* next_ptr = nullptr;
            while (head) {
                next_ptr = head;
                head = next_ptr->next;
                delete next_ptr;
            }
        }

        DoublyLinkedList(const DoublyLinkedList & dll) = delete;
        DoublyLinkedList& operator=(DoublyLinkedList const&) = delete;

        void append(Order* order) {
            if (tail == nullptr) {
                tail = order;
                head = order;
            } else {
                order->prev = tail;
                tail = order;
                tail->prev->next = order;
            }
            length++;
        }

        Order* pop_left() {
            if (head == nullptr) {
                return nullptr;
            }

            Order* res = head;

            if (head == tail) {
                head = nullptr;
                tail = nullptr;
            } else {
                head = head->next;
                head->prev = nullptr;
            }
            
            res->next = nullptr;
            length--;
            return res;
        }

        void remove(Order* order) {
            if (order == head) {
                head = order->next;
            } else {
                order->prev->next = order->next;
            }

            if (order == tail) {
                tail = order->prev;
            } else {
                order->next->prev = order->prev;
            }
            length--;
        }
};