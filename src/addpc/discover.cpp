#include "discover.h"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <packets.h>
#include <packet_io.h>

namespace addpc {

discover::discover(const std::string& listen, uint16_t port, const addp::mac_address& mac_address) :
    _io_service(),
    _listen(boost::asio::ip::address::from_string(listen), port),
    _socket(_io_service, _listen),
    _mcast_address(
            boost::asio::ip::address::from_string(addp::MCAST_IP_ADDRESS),
            addp::UDP_PORT),
    _sender_address(),
    _deadline(_socket.io_service()),
    _mac_address(mac_address),
    _count(0),
    _max_count(0),
    _timeout_ms(0),
    _verbose(false)
{
    // disable timer by seting expiry time to infinity
    _deadline.expires_at(boost::posix_time::pos_infin);
    check_timeout();
}

void discover::set_mac_address(const addp::mac_address& mac_address)
{
    _mac_address = mac_address;
}

void discover::set_mcast_address(const std::string& mcast_address, uint16_t port)
{
    _mcast_address = boost::asio::ip::udp::endpoint(
            boost::asio::ip::address::from_string(mcast_address),
            port);
}

void discover::set_timeout(ssize_t timeout_ms)
{
    _timeout_ms = timeout_ms;
}

void discover::set_max_count(ssize_t max_count)
{
    _max_count = max_count;
}

void discover::set_verbose(bool verbose)
{
    _verbose = verbose;
}

bool discover::run()
{
    addp::discovery_request request;

    boost::asio::ip::udp::endpoint endpoint(
        boost::asio::ip::address::from_string(addp::MCAST_IP_ADDRESS),
        addp::UDP_PORT);

    if(_verbose)
        std::cout << "sending to: " << _mcast_address << " packet: " << request
            << std::endl << std::endl;

    // set timeout from now
    _deadline.expires_from_now(boost::posix_time::milliseconds(_timeout_ms));

    _socket.async_send_to(
        boost::asio::buffer(request.raw()),
        _mcast_address,
        boost::bind(&discover::handle_send_to, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

    try
    {
        _io_service.run();
    }
    catch (const boost::system::system_error& error)
    {
        std::cerr << 
            str(boost::format("Error: %s (%d)")
                    % error.code().message()
                    % error.code().value())
            << std::endl;
        return false;
    }

    return true;
}

const std::list<addp::packet>& discover::packets() const
{
    return _packets;
}

void discover::handle_send_to(const boost::system::error_code& /*error*/, size_t /*bytes_sent*/)
{
    _socket.async_receive_from(
        boost::asio::buffer(_data, addp::MAX_UDP_MESSAGE_LEN), _sender_address,
        boost::bind(&discover::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void discover::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
    if(!error && bytes_recvd > 0)
    {
        addp::packet response(_data.data(), bytes_recvd);

        if(!response.parse_fields())
            std::cerr << "failed to parse fields!" << std::endl;

        std::cout << _sender_address << " " << response << std::endl;

        _packets.push_back(response);

        ++_count;
    }

    // count reached?
    if(_max_count && _count == _max_count)
        return;

    // timeout reached?
    if(error == boost::asio::error::operation_aborted)
        return;

    // other error?
    if(error)
        std::cerr << "error: " <<  error.value() << "(" << error.message() << ")"
            << " received: " << bytes_recvd << std::endl;

    // continue receiving
    _socket.async_receive_from(
        boost::asio::buffer(_data, addp::MAX_UDP_MESSAGE_LEN), _sender_address,
        boost::bind(&discover::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void discover::check_timeout()
{
    if(_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        _socket.cancel();
        _socket.io_service().stop();
        _deadline.expires_at(boost::posix_time::pos_infin);
    }
    _deadline.async_wait(boost::bind(&discover::check_timeout, this));
}

void discover::handle_receive(const boost::system::error_code& error, size_t bytes_recvd,
    boost::system::error_code* out_error, size_t* out_bytes_recvd)
{
    *out_error = error;
    *out_bytes_recvd = bytes_recvd;
}

} // namespace addpc
