//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdio.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <zlib.h>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
  virtual std::string get_user() 
  {
    return "";
  }

};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------


class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

  void set_name(std::string name)
  {
    name_ = name;
  }

  std::string get_name()
  {
    return name_;
  }

  std::string get_users()
  {
    std::string users = "";

    for (auto participant: participants_)
    {
      if(users == "")
      {
	users = participant->get_user();
      }
      else
      {
	users+=";" + participant->get_user();
      }
    }

    return users;
  }

  void store_text(std::string text)
  {
    text_.insert(text);
  }

  std::string get_text()
  {
    std::string text = "";

    for (auto string: text_)
    {
      if(text == "")
      {
        text = string;
      }
      else
      {
        text+=";" + string;
      }
    }

    text_.clear();
    return text;
  } 
    

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;

  std::set<std::string> text_;
  std::string name_;
};

std::set<chat_room*> chatrooms;

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
    uuid_ = boost::uuids::random_generator()();
  }

  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

  std::string get_user()
  {
    return to_string(uuid_) + " " + nick_;
  }

private:

  void add_time(chat_message& msg)
  {
    using namespace boost::posix_time;
    boost::posix_time::ptime time_local = boost::posix_time::second_clock::local_time();
    std::string add_data = to_iso_string(time_local) + "," + msg.body();
    msg.body_length(std::strlen(add_data.c_str())+1);
    std::memset(msg.body(), 0, msg.body_length());
    std::memcpy(msg.body(), add_data.c_str(), msg.body_length()-1);
  }
  
  void add_crc(chat_message& msg)
  {
    //Got this part of the code from cksum.cpp 
    unsigned int crc;
    
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const unsigned char*) msg.body(), msg.body_length());
    std::string add_data = std::to_string(crc) + "," + msg.body();
    msg.body_length(std::strlen(add_data.c_str())+1);
    std::memset(msg.body(), 0, msg.body_length());
    std::memcpy(msg.body(), add_data.c_str(), msg.body_length()-1);
  }
  
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
	    do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
	    std::string cmd, arg;
	    char body[512];
	    strcpy(body, read_msg_.body());
	
	    char *token = strtok(body, ",");
	
	    int count = 1;
	    while(token != NULL)
	    {
	      if(count == 3)
	      {
	        cmd = token;
	      }
	      else if(count == 4)
	      {
		arg = token;
		break;
	      }
	      token = strtok(NULL, ",");
	      count++;
	    }

	    if(cmd == "REQUUID")
	    {
	      chat_message msg;
        std::string data = cmd + "," + "12345789";
	      msg.body_length(std::strlen(data.c_str())+1); 
        std::memset(msg.body(), 0, msg.body_length());
	      std::memcpy(msg.body(), data.c_str(), msg.body_length()-1);
	      msg.encode_header();
        add_time(msg);
        add_crc(msg);
        std::cout << "Message body is " << msg.body() << " with length " << msg.body_length() << std::endl;
        deliver(msg);
	    }
	    else if(cmd == "NICK")
	    {
	      nick_ = arg;
	    }
	    else if(cmd == "REQCHATROOM")
	    {
	      std::string room_name = room_.get_name();
	      chat_message msg;
              std::string data = " , ," + cmd + "," + room_name;
	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);
	    }
	    else if(cmd == "REQCHATROOMS")
	    {
	      std::string names = "";
	      for (auto chat_room: chatrooms)
	      {
	        if(names == "")
		      {
      		  names = chat_room->get_name();
		      }
		      else
		      {
		        names+=";" + chat_room->get_name();
		      }
	     }

	      chat_message msg;
        std::string data = " , ," + cmd + "," + names;
	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
        deliver(msg);
	   }
	     else if(cmd == "NAMECHATROOM")
	     {
	      chat_room* chatroom = new chat_room();
	      chatroom->set_name(arg);
	      chatrooms.insert(chatroom);

	      chat_message msg;
              std::string data = " , ," + cmd + "," + arg;
	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);
	    }
	    else if(cmd == "CHANGECHATROOM")
	    {
	      for (auto chat_room: chatrooms)
	      {
	        std::string name = chat_room->get_name();
	        if(name == arg)
		{
		  room_.leave(shared_from_this());
		  chat_room->join(shared_from_this());
		  room_ = *chat_room;
		  
		  chat_message msg;
                  std::string data = " , ," + cmd + "," + name;
	          msg.body_length(std::strlen(data.c_str()));
	          std::memcpy(msg.body(), data.c_str(), msg.body_length());
	          msg.encode_header();
                  deliver(msg);
      		  break;
		}
	      }
	    }
	    else if(cmd == "SENDTEXT")
	    {
	      chat_message msg;
              std::string data = " , ," + cmd + "," + std::to_string(arg.length());
	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);

	      room_.store_text(to_string(uuid_) + " " + arg);
	      room_.deliver(read_msg_);
	    }
	    else if(cmd == "REQTEXT")
	    {
	      std::string text = room_.get_text();
	      chat_message msg;

              std::string data = " , ," + cmd + "," + text;
	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);
	    }
	    else if(cmd == "REQUSERS")
	    {
	      chat_message msg;
	      std::string users = room_.get_users();
              std::string data = " , ," + cmd + "," + users;
	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);
	    }
	    else
	    {
	      room_.deliver(read_msg_);
	    }
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
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
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;

  boost::uuids::uuid uuid_;
  std::string nick_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
    do_accept();
    chatrooms.insert(&room_);
    room_.set_name("The Lobby");
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<chat_session>(std::move(socket_), room_)->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }

    boost::asio::io_service io_service;

    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_service, endpoint);
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

