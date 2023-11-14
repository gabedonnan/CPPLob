#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <queue>
#include <stdexcept>

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

        void append(Order *order) {
            orders->push(order->id);
        }

        void append(int _id) {
            orders->push(_id);
        }

        int pop_left() {
            int val = orders->front();
            orders->pop();
            return val;
        }

        int get_head() {
            if (get_length()) {
                return orders->front();
            } else {
                throw std::length_error("Head attempted to be accessed from LimitLevel of length 0");
            }
        }

        int get_tail() {
            if (get_length()) {
                return orders->back();
            } else {
                throw std::length_error("Tail attempted to be accessed from LimitLevel of length 0");
            }
        }

        int get_length() {
            return orders->size();
        }
};

class LimitOrderBook final {
    private:
        std::map<int, LimitLevel*> bids;
        std::map<int, LimitLevel*> asks;
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
                orders.insert_or_assign(order->id, order);
                if (order_tree->count(order->price)) {
                    _append_limit((*order_tree)[order->price], order);
                } else {
                    order_tree->insert_or_assign(order->price, new LimitLevel(order));
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
                int head_order_id = best_value->get_head();

                if (orders.count(head_order_id)) {
                    head_order = orders[head_order_id];
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
        }

    public:
        std::unordered_map<int, Order*> orders;

        LimitOrderBook() {}

        LimitLevel* get_best_ask() {
            if (!asks.empty()) {
                return asks.rbegin()->second;
            } else {
                return nullptr;
            }
        }

        LimitLevel* get_best_bid() {
            if (!bids.empty()) {
                return bids.begin()->second;
            } else {
                return nullptr;
            }
        }

        void cancel(int id) {
            if (orders.count(id)) {
                Order* current_order = orders[id];
                std::map<int, LimitLevel*> *order_tree = _get_side(current_order->is_bid);

                if (order_tree->count(current_order->price)) {
                    LimitLevel *price_level = (*order_tree)[current_order->price];

                    price_level->quantity -= current_order->quantity;
                    if (price_level->quantity <= 0) {
                        // Delete the LimitLevel memory from the order tree
                        delete (*order_tree)[price_level->price];

                        // Delete mapping for LimitLevel from order tree
                        order_tree->erase(price_level->price);
                    }
                }
                delete current_order;
                orders.erase(id);
            }
        }

        int bid(int quantity, int price) {
            Order *order = new Order(true, quantity, price, order_id);

            order_id += 1;
            _add_order(order);
            return order_id - 1;
        }

        int ask(int quantity, int price) {
            Order *order = new Order(false, quantity, price, order_id);

            order_id += 1;
            _add_order(order);
            return order_id - 1;
        }

        int bid_quantity(int price) {
            // Returns the quantity of bids at a given price
            if (bids.count(price)) {
                return bids[price]->quantity;
            }
            return 0;
        }
};


// TESTS

void test_get_level_head() {
    Order* ord = new Order(false, 0, 0, 0);
    LimitLevel* lm = new LimitLevel(ord);
    std::cout << "test_get_level_head " << lm->get_head() << std::endl;
    delete ord;
    delete lm;
}

void test_order_count() {
    LimitOrderBook book = LimitOrderBook();
    book.bid(1, 1);
    std::cout << "test_order_count " << book.orders.count(0);
}



int main() {
    LimitOrderBook book = LimitOrderBook();
    book.cancel(0);
    std::cout << book.bid(1, 11) << book.ask(1, 11) << book.bid(1, 11) << book.ask(2, 11);
    book.cancel(3);
    book.bid(2, 10);
    book.ask(1, 9);
    book.bid(1,11);
    book.ask(2, 11);
    std::cout << book.get_best_ask();
    book.bid(1, 11);

    // std::map<int, int> tst = {{1, 2}};
    // tst.count(nullptr)

    // book.get_best_ask();
    // book.get_best_bid();
    // std::cout << book.bid_quantity(11);

    // test_get_level_head();
    // test_order_count();
}