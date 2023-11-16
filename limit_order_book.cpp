#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <chrono>
#include <mutex>
#include <thread>

struct Trader {
    std::string name;
    int balance = 0;
};

struct Order final {
    const bool is_bid;
    int quantity;
    const int price;
    const int id;
    Order (const bool _is_bid, int _quantity, const int _price, const int _id) : is_bid(_is_bid), quantity(_quantity), price(_price), id(_id) {}
};

class LimitLevel final {
    public:
        const int price;
        std::queue<int> *orders;
        int quantity;

        LimitLevel(Order *order) : price(order->price), quantity(order->quantity) {
            orders = new std::queue<int>();
            orders->push(order->id);
        }

        ~LimitLevel() {
            delete orders;
        }

        inline void append(Order *order) {
            orders->push(order->id);
        }

        inline void append(int _id) {
            orders->push(_id);
        }

        inline int pop_left() {
            int val = orders->front();
            orders->pop();
            return val;
        }

        inline int get_head() {
            if (get_length()) {
                return orders->front();
            } else {
                throw std::length_error("Head attempted to be accessed from LimitLevel of length 0");
            }
        }

        inline int get_tail() {
            if (get_length()) {
                return orders->back();
            } else {
                throw std::length_error("Tail attempted to be accessed from LimitLevel of length 0");
            }
        }

        inline int get_length() {
            return orders->size();
        }
};

class LimitOrderBook final {
    private:
        std::map<int, LimitLevel*> bids;
        std::map<int, LimitLevel*> asks;
        std::mutex order_id_mutex;
        std::mutex match_mutex;
        std::mutex cancel_mutex;
        std::mutex limit_level_add_mutex;
        int order_id = 0;

        inline std::map<int, LimitLevel*>* _get_side(bool is_bid) {
            // Return pointer to the bid or ask tree based on whether an order is a bid or ask
            return (is_bid) ? &bids : &asks;
        }

        inline int _pop_limit(LimitLevel *limit_level) {
            int res = limit_level->pop_left();
            limit_level->quantity -= orders.at(res)->quantity;
            return res;
        }

        inline void _append_limit(LimitLevel *limit_level, Order *order) {
            limit_level->append(order);
            limit_level->quantity += order->quantity;
        }

        inline void _add_order(Order *order) {
            std::map<int, LimitLevel*> *order_tree = _get_side(order->is_bid);
            LimitLevel *best_ask = get_best_ask();
            LimitLevel *best_bid = get_best_bid();

            if (order->is_bid && best_ask != nullptr && best_ask->price <= order->price) {
                _match_orders(order, best_ask);
                return;
            } else if (!order->is_bid && best_bid != nullptr && best_bid->price >= order->price) {
                _match_orders(order, best_bid);
                return;
            }

            if (order->quantity > 0) {
                orders.insert_or_assign(order->id, order);
                limit_level_add_mutex.lock();
                if (order_tree->count(order->price)) {
                    _append_limit(order_tree->at(order->price), order);
                } else {
                    order_tree->insert_or_assign(order->price, new LimitLevel(order));
                }
                limit_level_add_mutex.unlock();
            }
        }

        void _match_orders(Order *order, LimitLevel *best_value) {
            if (best_value == nullptr) {
                return;
            }

            // int order_multiplier = (order->is_bid) ? 1 : -1;
            // int matched_order_multiplier = (order->is_bid) ? -1 : 1;
            Order* head_order = nullptr;
            match_mutex.lock();
            while (best_value->quantity > 0 && order->quantity > 0 && best_value->get_length() > 0) {
                // The value of the head of the LimitLevel's linked list
                int head_order_id = best_value->get_head();

                if (orders.count(head_order_id)) {
                    head_order = orders.at(head_order_id);
                } else {
                    // Already manages the deletion of loose pointers
                    best_value->pop_left();
                    continue;
                }
                
                if (order->quantity <= head_order->quantity) {
                    head_order->quantity -= order->quantity;
                    best_value->quantity -= order->quantity;
                    order->quantity = 0;
                } else {
                    order->quantity -= head_order->quantity;
                    best_value->quantity -= head_order->quantity;
                    head_order->quantity = 0;
                }

                if (order->quantity == 0 && orders.count(order->id)) {
                    cancel(order->id);
                }
                
                if (head_order->quantity == 0 && orders.count(head_order_id)) {
                    _pop_limit(best_value);
                    orders.erase(head_order_id);
                }

                if (best_value->quantity == 0) {
                    std::map<int, LimitLevel*> *order_tree = _get_side(!(order->is_bid));
                    if (order_tree->count(best_value->price)) {
                        order_tree->erase(best_value->price);
                    }
                }
            }
            match_mutex.unlock();
        }

    public:
        std::unordered_map<int, Order*> orders;

        LimitOrderBook() {}

        inline LimitLevel* get_best_ask() {
            if (!asks.empty()) {
                return asks.rbegin()->second;
            } else {
                return nullptr;
            }
        }

        inline LimitLevel* get_best_bid() {
            if (!bids.empty()) {
                return bids.begin()->second;
            } else {
                return nullptr;
            }
        }

        inline void cancel(int id) {
            cancel_mutex.lock();
            if (orders.count(id)) {
                Order* current_order = orders.at(id);
                std::map<int, LimitLevel*> *order_tree = _get_side(current_order->is_bid);
                
                if (order_tree->count(current_order->price)) {
                    LimitLevel *price_level = order_tree->at(current_order->price);

                    price_level->quantity -= current_order->quantity;
                    if (price_level->quantity <= 0) {
                        // Delete mapping for LimitLevel from order tree
                        order_tree->erase(price_level->price);
                        // Delete the LimitLevel memory from the order tree
                        delete price_level;
                    }
                }
                delete current_order;
                orders.erase(id);
            }
            cancel_mutex.unlock();
        }

        inline int bid(int quantity, int price) {
            order_id_mutex.lock();

            Order *order = new Order(true, quantity, price, order_id);
            int _oid = order_id;  // Save order id from order
            order_id += 1;
            order_id_mutex.unlock();

            _add_order(order);
            return _oid;  // Dont return order->id in case it has been deleted
        }

        inline int ask(int quantity, int price) {
            order_id_mutex.lock();

            Order *order = new Order(false, quantity, price, order_id);
            int _oid = order_id;  // Save order id from order
            order_id += 1;
            order_id_mutex.unlock();

            _add_order(order);
            return _oid;  // Dont return order->id in case it has been deleted
        }

        inline int bid_quantity(int price) {
            // Returns the quantity of bids at a given price
            if (bids.count(price)) {
                return bids[price]->quantity;
            }
            return 0;
        }

        inline int update(int id, int quantity) {
            int new_id = -1;
            if (quantity == 0) {
                cancel(id);
                new_id = id;
            } else if (orders.count(id)) {
                std::map<int, LimitLevel*> *order_tree = _get_side(orders.at(id)->is_bid);
                Order* to_update = orders.at(id);

                int quantity_difference = to_update->quantity - quantity;

                if (quantity_difference >= 0) {
                    // If quantity is being decreased maintain order in price-time priority
                    to_update->quantity = quantity;
                    if (order_tree->count(to_update->price)) {
                        order_tree->at(to_update->price)->quantity -= quantity_difference;
                    }
                    new_id = id;

                } else {
                    // If quantity being increased move the order to the back of the queue
                    if (orders.at(id)->is_bid) {
                        new_id = bid(quantity, to_update->price);
                    } else {
                        new_id = ask(quantity, to_update->price);
                    }
                    cancel(id);

                }
            }
            return new_id;
        }
};


// TESTS

void test_adding() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++) {
        book.ask(1, 1);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void test_matching() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    book.bid(1,1);
    for (int i = 0; i < 10000000; i++) {
        book.bid(1,1);
        book.ask(1,1);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void test_cancellation() {
    LimitOrderBook book = LimitOrderBook();
    for (int j = 0; j < 10000000; j++) {
        book.bid(1, 1);
    }
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++) {
        book.cancel(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void test_update_decreasing() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    book.bid(10000001, 1);
    for (int i = 0; i < 10000000; i ++) {
        book.update(0, 10000001 - i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void test_update_increasing() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    book.bid(1, 1);
    for (int i = 0; i < 10000000; i ++) {
        book.update(i, 2 + i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void thread_bidder(LimitOrderBook* book) {
    for (int i = 0; i < 10000000; i++) {
        book->bid(1, 1);
    }
}

void thread_asker(LimitOrderBook* book) {
    for (int i = 0; i < 10000000; i++) {
        book->ask(1, 1);
    }
}

void test_threaded_add_cancel() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1{thread_bidder, &book};
    std::thread t2{thread_asker, &book};
    t1.join();
    t2.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}



int main() {
    test_matching();
    test_adding(); 
    test_cancellation();
    test_update_decreasing();
    test_update_increasing();

    test_threaded_add_cancel();


    // std::map<int, int> tst = {{1, 2}};
    // tst.count(nullptr)

    // book.get_best_ask();
    // book.get_best_bid();
    // std::cout << book.bid_quantity(11);

    // test_get_level_head();
    // test_order_count();
}