#include "doubly_linked_list.h"
#include <map>
#include <unordered_map>
#include <string>
#include <iostream>

struct Trader {
    std::string name;
    int balance = 0;
};

struct Order final {
    bool is_bid = false;
    int quantity = 0;
    int price = 0;
    int id = 0;
};

class LimitLevel final {
    public:
        int price;
        DoublyLinkedList *orders;
        int quantity;

        LimitLevel(Order *order) {
            price = order->price;
            orders = new DoublyLinkedList();
            orders->append(order->id);
            quantity = order->quantity;
        }

        void append(Order *order) {
            orders->append(order->quantity);
        }

        void append(int _id) {
            orders->append(_id);
        }

        int pop_left() {
            return orders->pop_left()->value;
        }
};

class LimitOrderBook {
    private:
        std::map<int, LimitLevel*> bids;
        std::map<int, LimitLevel*> asks;
        std::unordered_map<int, Order*> orders;
        int order_id = 0;

        std::map<int, LimitLevel*>* _get_side(bool is_bid) {
            // Return pointer to the bid or ask tree based on whether an order is a bid or ask
            return (is_bid) ? &bids : &asks;
        }

        int _pop_limit(LimitLevel *limit_level) {
            int res = limit_level->pop_left();
            limit_level->quantity -= orders[res]->quantity;
            return res;
        }

        void _append_limit(LimitLevel *limit_level, Order *order) {
            limit_level->append(order);
            limit_level->quantity += order->quantity;
        }

        void _add_order(Order *order) {
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
                orders[order->id] = order;
                if (order_tree->count(order->price)) {
                    _append_limit((*order_tree)[order->price], order);
                } else {
                    (*order_tree)[order->price] = new LimitLevel(order);
                }
            }
        }
    public:
        LimitOrderBook() {}

        LimitLevel* get_best_ask() {
            if (!asks.empty() ) {
                return asks[asks.rbegin()->first];
            } else {
                return nullptr;
            }
        }

        LimitLevel* get_best_bid() {
            if (!bids.empty()) {
                return bids[bids.begin()->first];
            } else {
                return nullptr;
            }
        }

        void _match_orders(Order *order, LimitLevel *best_value) {
            if (best_value == nullptr) {
                return;
            }

            // int order_multiplier = (order->is_bid) ? 1 : -1;
            // int matched_order_multiplier = (order->is_bid) ? -1 : 1;

            Order head_order;
            while (best_value->quantity > 0 && order->quantity > 0) {
                // The value of the head of the LimitLevel's linked list
                int head_order_id = best_value->orders->head->value;

                if (orders.count(head_order_id)) {
                    head_order = *orders[head_order_id];
                } else {
                    best_value->pop_left();
                    continue;
                }
                // break;
                if (order->quantity <= head_order.quantity) {
                    head_order.quantity -= order->quantity;
                    best_value->quantity -= order->quantity;
                    order->quantity = 0;
                } else {
                    best_value->quantity -= head_order.quantity;
                    order->quantity -= head_order.quantity;
                    head_order.quantity = 0;
                }

                if (order->quantity == 0 && orders.count(order->id)) {
                    cancel(order->id);
                }

                if (head_order.quantity == 0 && orders.count(head_order_id)) {
                    _pop_limit(best_value);
                    orders.erase(head_order_id);
                }
            }
        }

        void cancel(int id) {
            if (orders.count(id)) {
                std::map<int, LimitLevel*> *order_tree = _get_side(orders[id]->is_bid);
                LimitLevel *price_level = (*order_tree)[orders[id]->price];
                price_level->quantity -= orders[id]->quantity;
                if (price_level->quantity <= 0) {
                    delete (*order_tree)[price_level->price];
                    order_tree->erase(price_level->price);
                }
                delete orders[id];
                orders.erase(id);
            }
        }

        int bid(int quantity, int price) {
            Order *order = new Order();
            order->is_bid = true;
            order->quantity = quantity;
            order->price = price;
            order->id = order_id;

            order_id += 1;
            _add_order(order);
            return order_id - 1;
        }

        int ask(int quantity, int price) {
            Order *order = new Order();
            order->is_bid = false;
            order->quantity = quantity;
            order->price = price;
            order->id = order_id;

            order_id += 1;
            _add_order(order);
            return order_id - 1;
        }

        int bid_quantity() {
            return bids[100]->quantity;
        }
};

int main() {
  Order order;
  LimitOrderBook book = LimitOrderBook();
  book.bid(10, 10);
  book.ask(10, 10);
  book.bid(100, 100);
  book.ask(10, 100);
  book.ask(30, 100);
  std::cout << book.bid_quantity();
}