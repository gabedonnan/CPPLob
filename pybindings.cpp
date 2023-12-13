#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include "limit_order_book.hpp"

// Command: c++ -O3 -Wall -shared -std=c++17 -fPIC $(python3 -m pybind11 --includes)  pybindings.cpp -o BristolMatchingEngine$(python3-config --extension-suffix)

namespace py = pybind11;

PYBIND11_MODULE(BristolMatchingEngine, m) {
    py::class_<Trader>(m, "Trader")
        .def(py::init<const bool, const bool, const int, const int>())
        .def("get_order", &Trader::get_order)
        .def("get_order_quantity", &Trader::get_order_quantity)
        .def("get_order_price", &Trader::get_order_price)
        .def_readonly("id", &Trader::id)
        .def_readonly("is_bidder", &Trader::is_bidder)
        .def_readonly("is_asker", &Trader::is_asker)
        .def("__repr__", &Trader::to_str)
        .def("__str__", &Trader::to_str);


    py::class_<std::vector<Trader>>(m, "TraderVector")
        .def(py::init<>())
        .def("clear", &std::vector<Trader>::clear)
        .def("pop_back", &std::vector<Trader>::pop_back)
        .def("append", [](std::vector<Trader> &v, Trader &t) { v.push_back(t); })
        .def("__len__", [](const std::vector<Trader> &v) { return v.size(); })
        .def("__iter__", [](std::vector<Trader> &v) {
            return py::make_iterator(v.begin(), v.end());
        }, py::keep_alive<0, 1>())
        .def("__getitem__", [](const std::vector<Trader> &v, const int idx) { return (idx > -1) ? v.at(idx) : v.at(v.size() + idx); }); 


    // py::class_<std::vector<Transaction>>(m, "TransactionVector")
    //     .def(py::init<>())
    //     .def("clear", &std::vector<Transaction>::clear)
    //     .def("pop_back", &std::vector<Transaction>::pop_back)
    //     .def("__len__", [](const std::vector<Transaction> &v) { return v.size(); })
    //     .def("__iter__", [](std::vector<Transaction> &v) {
    //         return py::make_iterator(v.begin(), v.end());
    //     }, py::keep_alive<0, 1>())
    //     .def("__getitem__", [](const std::vector<Transaction> &v, const int idx) { return (idx > -1) ? v.at(idx) : v.at(v.size() + idx); }); 


    py::class_<LimitOrderBook>(m, "LimitOrderBook")
        .def(py::init<>())
        .def("get_best_ask", &LimitOrderBook::get_best_ask)
        .def("get_best_bid", &LimitOrderBook::get_best_bid)
        .def(
            "bid", 
            &LimitOrderBook::bid, 
            "Creates a bid (buy order) on the limit order book from the specified trader at specified quantity and price.\nReturns assigned order id.", 
            py::arg("quantity"),
            py::arg("price"),
            py::arg("trader_id")=0
            )
        .def(
            "ask", 
            &LimitOrderBook::ask,
            "Creates an ask (sell order) on the limit order book from the specified trader at specified quantity and price.\nReturns assigned order id.", 
            py::arg("quantity"),
            py::arg("price"),
            py::arg("trader_id")=0
            )
        .def("cancel", &LimitOrderBook::cancel)
        .def("update", &LimitOrderBook::update)
        .def("run_experiment", &LimitOrderBook::run_experiment)
        .def("__repr__", &LimitOrderBook::__repr__)
        .def("__str__", &LimitOrderBook::__repr__)
        .def("get_executed_transactions", [](const LimitOrderBook &lob) { return lob.executed_transactions; });
    
    // py::class_<Transaction>(m, "Transaction")
    //     .def(py::init<const int, const int, const int, const int>())
    //     .def_readonly("taker_id", &Transaction::taker_id)
    //     .def_readonly("maker_id", &Transaction::maker_id)
    //     .def_readonly("price", &Transaction::price)
    //     .def_readonly("quantity", &Transaction::quantity)
    //     .def("__repr__", &Transaction::to_str)
    //     .def("__str__", &Transaction::to_str);

}