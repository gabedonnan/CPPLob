#include "doubly_linked_list.hpp"
#include <map>
#include <unordered_map>
#include <iostream>
#include <deque>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstdlib>

struct Transaction {
    const int taker_trader_id;
    const int maker_trader_id;
    const int transaction_price;
};


class LimitLevel final {
    public:
        const int price;
        DoublyLinkedList orders;
        int quantity;

        LimitLevel(Order *order) : price(order->price), quantity(order->quantity) {
            orders.append(order);
        }

        inline void append(Order *order) {
            quantity += order->quantity;
            orders.append(order);
        }

        inline Order* pop_left() {
            quantity -= get_head()->quantity;
            return orders.pop_left();
        }

        inline void remove(Order* order) {
            quantity -= order->quantity;
            orders.remove(order);
        }

        inline Order* get_head() {
            return orders.head;
        }

        inline Order* get_tail() {
            return orders.tail;
        }

        inline int get_length() {
            return orders.length;
        }
};

class LimitOrderBook final {
    private:
        std::map<int, LimitLevel*> bids;
        std::map<int, LimitLevel*> asks;
        std::unordered_map<int, Order*> orders;

        std::deque<Transaction> executed_transactions;  // Use deque so it can be iterated over

        int order_id = 0;

        inline std::map<int, LimitLevel*>* _get_side(bool is_bid) {
            // Return pointer to the bid or ask tree based on whether an order is a bid or ask
            return (is_bid) ? &bids : &asks;
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

            if (order != nullptr && order->quantity > 0) {
                orders.insert(std::make_pair(order->id, order));  // Segfault here with map's at function

                if (order_tree->count(order->price)) {
                    order_tree->at(order->price)->append(order);
                } else {
                    order_tree->insert(std::make_pair(order->price, new LimitLevel(order)));
                }
            }
        }

        void _match_orders(Order *order, LimitLevel *best_value) {
            if (best_value == nullptr) {
                return;
            }

            // int order_multiplier = (order->is_bid) ? 1 : -1;
            // int matched_order_multiplier = (order->is_bid) ? -1 : 1;
            Order* head_order = nullptr;

            while (best_value->quantity > 0 && order->quantity > 0 && best_value->get_length() > 0) {
                // The value of the head of the LimitLevel's linked list
                Order* head_order = best_value->get_head();
                
                executed_transactions.push_back({order->trader_id, head_order->trader_id, head_order->price});

                if (order->quantity <= head_order->quantity) {
                    // Decrementing quantities
                    head_order->quantity -= order->quantity;
                    best_value->quantity -= order->quantity;
                    order->quantity = 0;
                } else {
                    // Decrementing quantities
                    order->quantity -= head_order->quantity;
                    best_value->quantity -= head_order->quantity;
                    head_order->quantity = 0;
                }

                if (order->quantity == 0 && orders.count(order->id)) {
                    cancel(order->id);
                }
                
                if (head_order->quantity == 0) {
                    orders.erase(head_order->id);
                    delete best_value->pop_left();  // Deletes head order as it is popped
                }

                if (best_value->quantity == 0) {    
                    std::map<int, LimitLevel*> *order_tree = _get_side(!(order->is_bid));
                    if (order_tree->count(best_value->price)) {
                        order_tree->erase(best_value->price);
                    }
                }
            }

            if (order != nullptr && order->quantity > 0) {
                _add_order(order);
            }
        }

    public:
        LimitOrderBook() {}

        inline LimitLevel* get_best_ask() {
            if (!asks.empty()) {
                return asks.begin()->second;
            } else {
                return nullptr;
            }
        }

        inline LimitLevel* get_best_bid() {
            if (!bids.empty()) {
                return bids.rbegin()->second;
            } else {
                return nullptr;
            }
        }

        inline void cancel(int id) {
            if (orders.count(id)) {
                Order* current_order = orders.at(id);
                std::map<int, LimitLevel*> *order_tree = _get_side(current_order->is_bid);
                
                if (order_tree->count(current_order->price)) {
                    LimitLevel *price_level = order_tree->at(current_order->price);
                    price_level->remove(current_order);

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
        }

        inline int bid(int quantity, const int price, const int trader_id = 0) {
            Order *order = new Order(true, quantity, price, order_id, trader_id);
            int _oid = order_id;  // Save order id from order
            order_id += 1;

            _add_order(order);
            return _oid;  // Dont return order->id in case it has been deleted
        }

        inline int ask(int quantity, const int price, const int trader_id = 0) {
            Order *order = new Order(false, quantity, price, order_id, trader_id);
            int _oid = order_id;  // Save order id from order
            order_id += 1;

            _add_order(order);
            return _oid;  // Dont return order->id in case it has been deleted
        }

        inline int update(int id, int quantity) {
            int new_id = id;
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
                    if (to_update->is_bid) {
                        new_id = bid(quantity, to_update->price, to_update->trader_id);
                    } else {
                        new_id = ask(quantity, to_update->price, to_update->trader_id);
                    }
                    cancel(id);

                }
            }
            return new_id;
        }

        void print_book() {
            std::cout << "BIDS\n";
            for (auto const& [key, val] : bids) {
                std::cout << val->quantity << " bids at price " << key << "\n";
            }
            std::cout << "ASKS\n";
            for (auto const& [key, val] : asks) {
                std::cout << val->quantity << " asks at price " << key << "\n";
            }
            std::cout << "\n";
        }

        void print_executions() {
            std::cout << "EXECUTED TXS\n";
            for (auto const& val : executed_transactions) {
                std::cout << "Maker ID: " << val.maker_trader_id << ", Taker ID: " << val.taker_trader_id << ", Trade Price: " << val.transaction_price << "\n";
            }
        }
};


// TESTS

void test_adding() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++) {
        book.ask(1, 1, 0);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void test_matching() {
    LimitOrderBook book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    book.bid(1,1, 0);
    for (int i = 0; i < 10000000; i++) {
        book.bid(1,1, 0);
        book.ask(1,1, 0);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
    // book.print_book();
}

void test_cancellation() {
    LimitOrderBook book = LimitOrderBook();
    for (int j = 0; j < 10000000; j++) {
        book.bid(1, 1, 0);
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
    book.bid(10000001, 1, 0);
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
    book.bid(1, 1, 0);
    for (int i = 0; i < 10000000; i ++) {
        book.update(i, 2 + i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
}

void test_random_orders() {
    LimitOrderBook book = LimitOrderBook();
    std::srand(std::time(nullptr)); // use current time as seed for random generator
    auto start = std::chrono::high_resolution_clock::now();
    int num_orders = 0;
    for (int i = 0; i < 100; i++) {
        int next_orders = (int)(std::rand() / 10000);
        num_orders += next_orders;
        for (int j = 0; j < next_orders; j++) {
            book.bid((int)(std::rand() / 100), std::rand(), 0);
        }
        // std::cout << num_orders;
        next_orders = (int)(std::rand() / 10000);
        num_orders += next_orders;
        for (int j = 0; j < next_orders; j++) {
            book.ask((int)(std::rand() / 100), std::rand(), 0);
        }
    }
    std::cout << num_orders << "\n";
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << duration.count() << " seconds taken\n";
    std::cout << book.get_best_bid()->price << " " << book.get_best_ask()->price << "\n";
}

void test_lob_accuracy() {
    LimitOrderBook book;
    book.print_book();

    book.bid(1,1, 0);
    book.print_book();

    book.bid(2,1, 1);
    book.print_book();

    book.ask(4, 1, 2);
    book.print_book();

    book.bid(5, 1, 3);
    book.ask(5, 2, 4);
    book.ask(1, 3, 3);
    book.print_book();
    
    book.bid(10, 5, 1);
    book.print_book();

    book.cancel(3);
    book.print_book();

    book.print_executions();
}

void run_test_benchmarks() {
    test_adding();
    test_matching();
    test_cancellation();
    test_update_decreasing();
    test_update_increasing();
    test_random_orders();
}

int main() {
    run_test_benchmarks();    
}