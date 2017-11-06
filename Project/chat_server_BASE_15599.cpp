//
// chat_server.cpp
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
#include <list>
#include <memory>
#include <string>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;
// using namespace std;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
    chat_participant(std::string p_name)
  {
      name = p_name;
  }
  chat_participant()
  {
  }
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;

  std::string get_name()
  {
      return name;
  }

protected:
    std::string name;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    chat_room(std::string c_name)
  {
      name = c_name;
  }
    chat_room()
  {

  }

  std::string get_name()
  {
      return name;
  }

  std::list<std::string> get_users()
  {
       std::list<std::string> users;
      std::set<chat_participant_ptr>::iterator it;
        for (it = participants_.begin(); it != participants_.end(); ++it)
        {
            users.push_back((*it)->get_name());
        }
        return users;
  }

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

  chat_participant_ptr get_participant(std::string name)
  {
      std::set<chat_participant_ptr>::iterator it;
        for (it = participants_.begin(); it != participants_.end(); ++it)
        {
            if((*it)->get_name() == name)
            {
                return (chat_participant_ptr)(*it);
            }
        }
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

  chat_message_queue get_messages()
  {
      return recent_msgs_;
  }

private:
    std::string name;
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room, std::string p_name)
  : socket_(std::move(socket)),
    room_(room)
  {
      name = p_name;
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

private:
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
            room_.deliver(read_msg_);
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
  }

  std::string add_client(std::string nick)
  {
        std::list<std::string>::const_iterator iterator;
        for (iterator = clients.begin(); iterator != clients.end(); ++iterator)
        {
            if(*iterator == nick)
            {
                return "Nickname \'" + nick + "\' already exists!";
            }
        }
        clients.push_back(nick);
        return "";
  }

  std::string add_chatroom(std::string name)
  {
        std::list<chat_room*>::const_iterator it;
        for (it = chatrooms.begin(); it != chatrooms.end(); ++it)
        {
            if((*it)->get_name() == name)
            {
                return "Chatroom \'" + name + "\' already exists!";
            }
        }
        chatrooms.push_back(new chat_room(name));
        return "";
  }

  void join_chatroom(tcp::socket socket, std::string c_name, std::string p_name)
  {
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if((*iterator)->get_name() == c_name)
            {
                chat_participant_ptr participant = (*iterator)->get_participant(p_name);
                if(participant == NULL)
                {
                    (*iterator)->join(new chat_session(socket, *iterator, p_name));
                }
                break;
            }
        }
  }

  void leave_chatroom(std::string c_name, std::string p_name)
  {
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if((*iterator)->get_name() == c_name)
            {
                chat_participant_ptr participant = (*iterator)->get_participant(p_name);
                if(participant != NULL)
                {
                    (*iterator)->leave(participant);
                }
                break;
            }
        }
  }

  void delete_chatroom(std::string name)
  {
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if((*iterator)->get_name() == name)
            {
                chatrooms.remove(*iterator);
            }
        }
  }

  void delete_client(std::string nick)
  {
        std::list<std::string>::const_iterator iterator;
        for (iterator = clients.begin(); iterator != clients.end(); ++iterator)
        {
            if(*iterator == nick)
            {
                clients.remove(nick);
            }
        }
  }

  std::list<std::string> req_chatrooms()
  {
      std::list<std::string> chatrooom_names;
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            chatrooom_names.push_back((*iterator)->get_name());
        }
      return chatrooom_names;
  }

  std::list<std::string> req_users(std::string c_name)
  {
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if(c_name == (*iterator)->get_name())
            {
                return (*iterator)->get_users();
            }
        }
      return 0;
  }

  std::list<std::string> reqtext(std::string c_name)
  {
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if(c_name == (*iterator)->get_name())
            {
                return (*iterator)->get_messages();
            }
        }
      return 0;
  }
private:
    std::list<std::string> clients;
    std::list<chat_room*> chatrooms;

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
