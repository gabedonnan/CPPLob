#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include "doubly_linked_list.hpp"
#include <map>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>


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

            if (order != nullptr) {
                if (order->quantity > 0 && order->order_type != OrderType::fill_and_kill) {
                    orders.insert(std::make_pair(order->id, order));
                    
                    // Insert order id to trader's orders made
                    if (order_tree->count(order->price)) {
                        order_tree->at(order->price)->append(order);
                    } else {
                        order_tree->insert(std::make_pair(order->price, new LimitLevel(order)));
                    }
                } else {
                    // If order has zero quantity left or is fill and kill, delete it
                    delete order;
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

        inline int bid(int quantity, const int price, const OrderType order_type) {
            if (price >= 0 && quantity > 0) {
                Order *order = new Order(true, quantity, price, order_id, order_type);
                int _oid = order_id;  // Save order id from order
                order_id++;

                _add_order(order);
                return _oid;  // Dont return order->id in case it has been deleted
            } else {
                return -1;  // Returns -1 if invalid order
            }
        }

        inline int market_bid(int quantity) {
            if (quantity > 0) {
                Order *order = new Order(true, quantity, INT32_MAX, order_id, OrderType::market);
                int _oid = order_id;
                order_id++;

                _add_order(order);
                return _oid;
            } else {
                return -1;
            }
        }

        inline int ask(int quantity, const int price, const OrderType order_type) {
            if (price >= 0 && quantity > 0) {
                Order *order = new Order(false, quantity, price, order_id, order_type);
                int _oid = order_id;  // Save order id from order
                order_id++;

                _add_order(order);
                return _oid;  // Dont return order->id in case it has been deleted
            } else {
                return -1;  // Returns -1 if invalid order
            }
        }

        inline int market_ask(int quantity) {
            if (quantity > 0) {
                Order *order = new Order(false, quantity, 0, order_id, OrderType::market);
                int _oid = order_id;
                order_id++;

                _add_order(order);
                return _oid;
            } else {
                return -1;
            }
        }

        inline void update(int id, int quantity) {
            if (quantity == 0) {
                cancel(id);
            } else if (orders.count(id)) {
                LimitLevel* level = nullptr;
                Order* to_update = orders.at(id);
                std::map<int, LimitLevel*> *order_tree = _get_side(to_update->is_bid);

                if (order_tree->count(to_update->price)) {
                    level = order_tree->at(to_update->price);
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
                    level->remove(to_update);
                    
                    to_update->quantity = quantity;

                    level->append(to_update);
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
};

#endif