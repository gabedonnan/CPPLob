#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <cinttypes>
#include "limit_order_book.hpp"
#include <disruptorplus/ring_buffer.hpp>
#include <disruptorplus/single_threaded_claim_strategy.hpp>
#include <disruptorplus/spin_wait_strategy.hpp>
#include <disruptorplus/sequence_barrier.hpp>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

std::vector<std::string> split (const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> _ws;
    beast::flat_buffer _buffer;

public:
    // Take ownership of the socket
    explicit session(tcp::socket&& socket) : _ws(std::move(socket)) {}

    // Get on the correct executor
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(_ws.get_executor(),
            beast::bind_front_handler(
                &session::on_run,
                shared_from_this()));
    }

    // Start the asynchronous operation
    void on_run()
    {
        // Set suggested timeout settings for the websocket
        _ws.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        _ws.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));
        // Accept the websocket handshake
        _ws.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        _ws.async_read(
            _buffer,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            return fail(ec, "read");

        // Parse buffer data to get commands for 


        // Echo the message
        _ws.text(_ws.got_text());
        _ws.async_write(
            _buffer.data(),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void parse_buffer(beast::error_code ec) {
        // Call the requisite function
        // Bid = B {quantity} {price}
        // Ask = A {quantity} {price}
        // Cancel = C {id}
        // Update = U {id} {quantity}
        // 
        std::string buffer_data(net::buffers_begin(_buffer), net::buffers_begin(_buffer) + _buffer.data().size());
        // Write data to ringbuff
        
    }

    void on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        _buffer.consume(_buffer.size());

        // Do another read
        do_read();
    }
};


class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& _ioc;
    tcp::acceptor _acceptor;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint
    ) : _ioc(ioc), _acceptor(ioc)
    {
        beast::error_code ec;

        // Open acceptor
        _acceptor.open(endpoint.protocol(), ec);

        if (ec) {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        _acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            fail(ec, "set_option");
            return;
        }
        
        // Bind to the server address
        _acceptor.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }

        _acceptor.listen(
            net::socket_base::max_listen_connections, ec
        );
        if (ec) {
            fail(ec, "listen");
            return;
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        _acceptor.async_accept(
            net::make_strand(_ioc),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()
            )
        );
    }

    void on_accept(tcp::socket socket, beast::error_code ec) {
        if (ec) {
            fail(ec, "accept");
        } else {
            std::make_shared<session>(std::move(socket))->run();
        }

        do_accept();
    }
};

struct Event {
    uint8_t command;
    uint32_t field_one;
    uint32_t field_two;
    OrderType order_type;
    uint32_t trader_id;
}


void consumer(
    &disruptorplus::ring_buffer<Event> buffer,
    &disriptorplus::spin_wait_strategy wait_strategy, 
    &disruptorplus::multi_threaded_claim_strategy<disruptorplus::spin_wait_strategy> claim_strategy,
    &disruptorplus::sequence_barrier<disruptorplus::spin_wait_strategy> consumed
) {
    // Setup
    LimitOrderBook lob();
    disruptorplus::sequence_t next_to_read = 0;

    while (true) {
        // Consume stuff from disruptor
        disruptorplus::sequence_t available = claim_strategy.wait_until_published(next_to_read);

        do {
            auto& event = buffer[next_to_read];
            // Call LOB functions
            switch (event.command) {
                case 0:
                    lob.bid(event.field_one, event.field_two, event.order_type, event.trader_id);
                    break;
                case 1:
                    lob.ask(event.field_one, event.field_two, event.order_type, event.trader_id);
                    break;
                case 2:
                    lob.cancel(event.field_one, event.trader_id);
                    break;
                case 3:
                    lob.update(event.field_one, event.field_two, event.trader_id);
                    break;
            }
        } while (next_to_read++ != available);

        consumed.publish(available);
    }
}


int main() {
    const size_t buffer_size = 8192; // Must be power-of-two, set higher for higher throughput
    
    disruptorplus::ring_buffer<Event> buffer(buffer_size);
    
    disruptorplus::spin_wait_strategy wait_strategy;
    disruptorplus::multi_threaded_claim_strategy<disruptorplus::spin_wait_strategy> claim_strategy(buffer_size, wait_strategy);
    disruptorplus::sequence_barrier<disruptorplus::spin_wait_strategy> consumed(wait_strategy);
    claim_strategy.add_claim_barrier(consumed);

    auto const address = net::ip::make_address("0.0.0.0");
    const unsigned short port = 8080;
    const int threads = 8;  // Number of threads

    net::io_context ioc{threads};

    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });

    std::thread consumer(consumer, std::ref(buffer), std::ref(wait_strategy), std::ref(claim_strategy), std::ref(consumed));

    ioc.run();
    consumer.join();

    return 0;
}


// switch (order_type_id) {
//     case 0:
//         order_type = OrderType.limit;
//         break;
//     case 1:
//         order_type = OrderType.fill_and_kill;
//         break;
//     case 2:
//         order_type = OrderType.market;
//         break;
//     case 3:
//         order_type = OrderType.immediate_or_cancel;
//         break;
//     case 4:
//         order_type = OrderType.post_only;
//         break;
//     default:
//         order_type = OrderType.limit;
// }