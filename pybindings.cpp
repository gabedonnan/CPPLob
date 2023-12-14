#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
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
            py::arg("order_type")
            )
        .def(
            "ask", 
            &LimitOrderBook::ask,
            "Creates an ask (sell order) on the limit order book from the specified trader at specified quantity and price.\nReturns assigned order id.", 
            py::arg("quantity"),
            py::arg("price"),
            py::arg("order_type")
            )
        .def(
            "market_bid",
            &LimitOrderBook::market_bid,
            "Buys n quantity at the best prices available on the LimitOrderBook",
            py::arg("quantity")
        )
        .def(
            "market_ask",
            &LimitOrderBook::market_ask,
            "Sells n quantity at the best prices available on the LimitOrderBook",
            py::arg("quantity")
        )
        .def("cancel", &LimitOrderBook::cancel)
        .def("update", &LimitOrderBook::update)
        .def("__repr__", &LimitOrderBook::__repr__)
        .def("__str__", &LimitOrderBook::__repr__);
    
    // py::class_<Transaction>(m, "Transaction")
    //     .def(py::init<const int, const int, const int, const int>())
    //     .def_readonly("taker_id", &Transaction::taker_id)
    //     .def_readonly("maker_id", &Transaction::maker_id)
    //     .def_readonly("price", &Transaction::price)
    //     .def_readonly("quantity", &Transaction::quantity)
    //     .def("__repr__", &Transaction::to_str)
    //     .def("__str__", &Transaction::to_str);

}