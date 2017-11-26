//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <thread>
#include <vector>
#include <algorithm>
#include <zlib.h>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time.hpp>

#include "chat_message.hpp"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
  chat_client(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator,void (*data_recv) (std::string S))
    : io_service_(io_service),
      socket_(io_service),data_recv_ ( data_recv )
  {
    do_connect(endpoint_iterator);
  }

  void write(const chat_message& msg)
  {
    io_service_.post(
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }

  void add_time(chat_message& msg)
  {
    using namespace boost::posix_time;
    boost::posix_time::ptime time_local = boost::posix_time::second_clock::local_time();
    std::string add_data = to_iso_string(time_local) + "," + msg.body();
    msg.body_length(std::strlen(add_data.c_str()));
    std::memcpy(msg.body(), add_data.c_str(), msg.body_length());
  }
  
  void add_crc(chat_message& msg)
  {
    //Got this part of the code from cksum.cpp 
    unsigned int crc;
    
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const unsigned char*) msg.body(), msg.body_length());
    std::string add_data = std::to_string(crc) + "," + msg.body();
    msg.body_length(std::strlen(add_data.c_str()));
    std::memcpy(msg.body(), add_data.c_str(), msg.body_length());
  }
  
  void nick(std::string nick_name)
  {
    chat_message msg;
    std::string data = "NICK," + nick_name; 
    msg.body_length(std::strlen(data.c_str()));
    std::memcpy(msg.body(), data.c_str(), msg.body_length());
    add_time(msg);
    add_crc(msg);
    msg.encode_header();
    this->write(msg);
  }
  
  void requuid()
  {
    chat_message msg;
    std::string data = "REQUUID";
    msg.body_length(std::strlen(data.c_str()));
    std::memcpy(msg.body(), data.c_str(), msg.body_length());
    add_time(msg);
    add_crc(msg);
    msg.encode_header();
    this->write(msg);
  }

private:
    
  void set_uuid(std::string id)
  {
    this->uuid = id;
    std::cout << this->uuid;
  }

  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }

  template<typename Out>
  void split_out(const std::string &s, char delim, Out result)
  {
    std::cout << "calling split_out ----- " << s << std::endl;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
      std::cout << "in while loop" << std::endl;
      *(result++) = item;
    }
  }
  
  std::vector<std::string> split(const std::string &data, char delim)
  {
    std::cout << "calling split" << std::endl;
    std::vector<std::string> tokens;
    split_out(data, delim, std::back_inserter(tokens));
    return tokens;
  }
  
  void do_read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::cout << "read msg body is ---- " <<read_msg_.body() << std::endl;
            /*std::vector<std::string> tokens = split(read_msg_.body(), ',');
            if((tokens.at(2)).compare("REQUUID") == 0)
            {
              if(tokens.size() == 4)
              {
                this->set_uuid(tokens.at(3));    
              }
            }*/
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            data_recv_ ( read_msg_.body() );
            do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  void (*data_recv_) (std::string S);
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  std::string participant_name;
  std::string uuid;
};
#ifdef XXX
int mainxx(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
    chat_client c(io_service, endpoint_iterator);

    std::thread t([&io_service](){ io_service.run(); });

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      chat_message msg;
      msg.body_length(std::strlen(line));
      std::memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
#endif
