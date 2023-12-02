#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include "doubly_linked_list.hpp"
#include <map>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <random>

static int _cur_tid = 0;

class Trader {
    public:
        const bool is_bidder;
        const bool is_asker;
        const int id;

        Trader(const bool _bids, const bool _asks) : is_bidder(_bids), is_asker(_asks), id(_cur_tid) {
            _cur_tid++;
        }

        const int get_order_quantity(const bool is_bid) {
            return 1;
        }

        const int get_order_price(const bool is_bid) {
            return 1;
        }

        std::pair<const int, const int> get_order(const bool is_bid) {
            return std::make_pair(get_order_quantity(is_bid), get_order_price(is_bid));
        }

        std::string to_str() {
            return "Trader(id=" + std::to_string(id) + ", is_bidder=" + std::to_string(is_bidder) + ", is_asker=" + std::to_string(is_asker) + ")";
        }
};


struct Transaction {
    const int taker_id;
    const int maker_id;
    const int quantity;
    const int price;
    Transaction(const int _taker_id, const int _maker_id, const int _tx_quantity, const int _tx_price) 
        : taker_id(_taker_id), maker_id(_maker_id), quantity(_tx_quantity), price(_tx_price) {}
    std::string to_str() {
        return "Transaction(maker_id=" + std::to_string(maker_id) + ", taker_id=" + std::to_string(taker_id) + ", quantity=" + std::to_string(quantity) + ", price=" + std::to_string(price) +  ")";
    }
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

            while (best_value->quantity > 0 && order->quantity > 0 && best_value->get_length() > 0) {
                // The value of the head of the LimitLevel's linked list
                Order* head_order = best_value->get_head();

                if (order->quantity <= head_order->quantity) {
                    executed_transactions.push_back({order->trader_id, head_order->trader_id, order->quantity, head_order->price});
                    // Decrementing quantities
                    head_order->quantity -= order->quantity;
                    best_value->quantity -= order->quantity;
                    order->quantity = 0;
                } else {
                    executed_transactions.push_back({order->trader_id, head_order->trader_id, head_order->quantity, head_order->price});
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
        std::vector<Transaction> executed_transactions; 

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
            if (price > 0 && quantity > 0) {
                Order *order = new Order(true, quantity, price, order_id, trader_id);
                int _oid = order_id;  // Save order id from order
                order_id += 1;

                _add_order(order);
                return _oid;  // Dont return order->id in case it has been deleted
            } else {
                return -1;  // Returns -1 if invalid order
            }
        }

        inline int ask(int quantity, const int price, const int trader_id = 0) {
            if (price > 0 && quantity > 0) {
                Order *order = new Order(false, quantity, price, order_id, trader_id);
                int _oid = order_id;  // Save order id from order
                order_id += 1;

                _add_order(order);
                return _oid;  // Dont return order->id in case it has been deleted
            } else {
                return -1;  // Returns -1 if invalid order
            }
        }

        inline void update(int id, int quantity) {
            if (quantity == 0) {
                cancel(id);
            } else if (orders.count(id)) {
                Order* to_update = orders.at(id);
                std::map<int, LimitLevel*> *order_tree = _get_side(to_update->is_bid);
                if (order_tree->count(to_update->price)) {
                    LimitLevel* level = order_tree->at(to_update->price);
                } else {
                    return;
                }

                int quantity_difference = to_update->quantity - quantity;

                if (quantity_difference >= 0) {
                    // If quantity is being decreased maintain order in price-time priority
                    to_update->quantity = quantity;
                    level->quantity -= quantity_difference;
                } else {
                    // If quantity being increased move the order to the back of the queue
                    to_update->quantity = quantity;
                    level->quantity -= quantity_difference;  // This is a subtraction as qdiff will be negative

                    level->remove(to_update);
                    level->append(to_update)
                }
            }
        }

        std::string __repr__() {
            std::string res = "BIDS\n";
            for (auto const& [key, val] : bids) {
                res += std::to_string(val->quantity) + " bids at price " + std::to_string(key) + "\n";
            }
            res += "ASKS\n";
            for (auto const& [key, val] : asks) {
                res += std::to_string(val->quantity) + " asks at price " + std::to_string(key) + "\n";
            }
            return res;
        }

        void run_experiment(int start_time, int end_time, std::vector<Trader> traders) {
            for ( ; start_time < end_time; start_time++) {
                for (auto& trader : traders) {
                    if (trader.is_bidder) {
                        std::pair<const int, const int> func_results = trader.get_order(true);
                        bid(func_results.first, func_results.second, trader.id);
                    } 
                    
                    if (trader.is_asker) {
                        std::pair<const int, const int> func_results = trader.get_order(false);
                        ask(func_results.first, func_results.second, trader.id);
                    }
                }
            }
        }
};

#endif