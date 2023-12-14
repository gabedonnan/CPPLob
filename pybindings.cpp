#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <sstream>
#include <string>
#include "limit_order_book.hpp"

// Command: c++ -O3 -Wall -shared -std=c++17 -fPIC $(python3 -m pybind11 --includes)  pybindings.cpp -o BristolMatchingEngine$(python3-config --extension-suffix)

namespace py = pybind11;

PYBIND11_MODULE(BristolMatchingEngine, m) {
    py::enum_<OrderType>(m, "OrderType")
        .value("limit", OrderType::limit)
        .value("fill_and_kill", OrderType::fill_and_kill)
        .value("market", OrderType::market)
        .export_values();

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
            py::arg("order_type"),
            py::arg("trader_id")
            )
        .def(
            "ask", 
            &LimitOrderBook::ask,
            "Creates an ask (sell order) on the limit order book from the specified trader at specified quantity and price.\nReturns assigned order id.", 
            py::arg("quantity"),
            py::arg("price"),
            py::arg("order_type"),
            py::arg("trader_id")
            )
        .def(
            "market_bid",
            &LimitOrderBook::market_bid,
            "Buys n quantity at the best prices available on the LimitOrderBook",
            py::arg("quantity"),
            py::arg("trader_id")
        )
        .def(
            "market_ask",
            &LimitOrderBook::market_ask,
            "Sells n quantity at the best prices available on the LimitOrderBook",
            py::arg("quantity"),
            py::arg("trader_id")
        )
        .def("cancel", &LimitOrderBook::cancel)
        .def("update", &LimitOrderBook::update)
        .def("__repr__", &LimitOrderBook::__repr__)
        .def("__str__", &LimitOrderBook::__repr__)
        .def("get_executed_transactions", [](const LimitOrderBook &lob) { return lob.executed_transactions; });;


    py::class_<std::vector<Transaction>>(m, "TransactionVector")
        .def(py::init<>())
        .def("clear", &std::vector<Transaction>::clear)
        .def("pop", &std::vector<Transaction>::pop_back)
        .def("__len__", [](const std::vector<Transaction> &v) { return v.size(); })
        .def("__iter__", [](std::vector<Transaction> &v) {
            return py::make_iterator(v.begin(), v.end());
        }, py::keep_alive<0, 1>())
        .def("__getitem__", [](const std::vector<Transaction> &v, const int idx) { return (idx > -1) ? v.at(idx) : v.at(v.size() + idx); })
        .def("__repr__", [](const std::vector<Transaction> &v) { 
            std::stringstream ss; 
            ss << "[";
            for (auto tx : v) {
                ss << tx.to_str();
                ss << ", ";
            }
            ss << "]";
            return ss.str();
            })
        .def("__str__", [](const std::vector<Transaction> &v) { 
            std::stringstream ss; 
            ss << "[";
            for (auto tx : v) {
                ss << tx.to_str();
                ss << ", ";
            }
            ss << "]";
            return ss.str();
            }); 

    
    py::class_<Transaction>(m, "Transaction")
        .def(py::init<const int, const int, const int, const int>())
        .def_readonly("taker_id", &Transaction::trader_one)
        .def_readonly("maker_id", &Transaction::trader_two)
        .def_readonly("price", &Transaction::price)
        .def_readonly("quantity", &Transaction::quantity)
        .def("__repr__", &Transaction::to_str)
        .def("__str__", &Transaction::to_str);

}