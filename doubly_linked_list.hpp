#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

static int _cur_tid = 0;

class Trader {
    private:
        const int get_order_quantity(const bool is_bid) {
            // Simple definition to be inherited from
            return 1;
        }

        const int get_order_price(const bool is_bid) {
            // Simple definition to be inherited from
            return 1;
        }

    public:
        const bool is_bidder;
        const bool is_asker;
        const int start_time;
        const int end_time;
        const int id;
        std::set<int> order_ids;

        Trader(const bool _bids, const bool _asks, const int _start_time, const int _end_time) 
            : is_bidder(_bids), is_asker(_asks), id(_cur_tid), start_time(_start_time), end_time(_end_time) {}

        const int get_cancellation() {
            // If any orders need to be cancelled, send a cancellation request for it
            // Very simple base level implementation to always cancel order 0
            return 0;
        }

        std::pair<const int, const int> get_update() {
            // If any orders need to be updated, send an update request for it of form (ID, Quantity)
            // Very simple base level implementation to always update order 0 to quantity 0
            return std::make_pair(0, 0);
        }

        std::pair<const int, const int> get_order(const bool is_bid) {
            return std::make_pair(get_order_quantity(is_bid), get_order_price(is_bid));
        }

        std::string to_str() {
            return "Trader(id=" + std::to_string(id) + ", is_bidder=" + std::to_string(is_bidder) + ", is_asker=" + std::to_string(is_asker) + ")";
        }
};

enum class OrderType {
    limit,
    fill_and_kill,
    market,
    iceberg
};

struct Order final {
    const bool is_bid;
    int quantity;
    const int price;
    const int id;
    const int alternate_price;
    int alternate_quantity;
    const OrderType order_type;
    const Trader* trader_ptr;
    Order *prev;
    Order *next;
    Order (const bool _is_bid, int _quantity, const int _price, const int _id, const OrderType _order_type) 
        : is_bid(_is_bid), quantity(_quantity), price(_price), id(_id), order_type(_order_type), trader_ptr(nullptr), prev(nullptr), next(nullptr), alternate_quantity(0), alternate_price(0) {}
    
    Order (const bool _is_bid, int _quantity, const int _price, const int _id, const OrderType _order_type, int alternate_quantity, const int alternate_price)
        : is_bid(_is_bid), quantity(_quantity), price(_price), id(_id), order_type(_order_type), trader_ptr(nullptr), prev(nullptr), next(nullptr), alternate_quantity(alternate_quantity), alternate_price(alternate_price) {}
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

        inline void append(Order* order) {
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

        inline Order* pop_left() {
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

        inline void remove(Order* order) {
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

#endif