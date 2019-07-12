/*
    Websocket client to send a custom header to server
    "headers": {
        "Host": "host.com",
        ...
        ...
        ...
        "custom_header_name": "value_of_custom_header",
    },    
*/

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;


class custom_http_header
{
    std::string m_key;
    std::string m_value;

public:
    explicit
    custom_http_header(std::string key, std::string value)
        : m_key(key), m_value(value) {}

    template<bool isRequest, class Body, class Headers>
    void
    operator()(http::message<isRequest, Body, Headers>& m) const
    {
        m.set(m_key, m_value);
    }
};


// Sends a WebSocket message and prints the response
int main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if(argc != 4)
        {
            std::cerr <<
                "Usage: websocket-client-sync <host> <port> <text>\n" <<
                "Example:\n" <<
                "    websocket-client-sync echo.websocket.org 80 \"Hello, world!\"\n";
            return EXIT_FAILURE;
        }

        // Check command line arguments.
        auto const host = argv[1];
        auto const port = argv[2];
        auto const text = argv[3];

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver{ioc};

        // Non ssl
        // websocket::stream<tcp::socket> ws{ioc};

        // ssl
        ssl::context ctx{ssl::context::sslv23};
        ctx.set_verify_mode(ssl::verify_none);
        websocket::stream<ssl::stream<tcp::socket>> ws{ioc, ctx};


        std::cout << "Resolving.." << std::endl;
        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        std::cout << "Connecting" << std::endl;
        net::connect(ws.next_layer().next_layer(), results.begin(), results.end());

        // handshake ssl
        std::cout << "SSL handshake" << std::endl;
        ws.next_layer().handshake(ssl::stream_base::client);

        std::cout << "Setting our custom header" << std::endl;
        ws.set_option(
            websocket::stream_base::decorator(
                custom_http_header("custom_header_name", "value_of_custom_header")
        ));

        std::cout << "Handshake with Endpoint" << std::endl;
        // Put endpoint here if not '/'
        ws.handshake(host, "/");


        std::cout << "Writing" << std::endl;
        // Send the message
        ws.write(net::buffer(std::string(text)));

        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message into our buffer
        ws.read(buffer);

        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
