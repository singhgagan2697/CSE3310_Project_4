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
#include <string>
#include <set>
#include <utility>
#include <zlib.h>
#include <boost/asio.hpp>
<<<<<<< HEAD
#include <boost/shared_ptr.hpp>
=======
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time.hpp>
>>>>>>> jeet
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
<<<<<<< HEAD

  std::string get_name()
  {
      return name;
  }

protected:
    std::string name;
=======
  virtual std::string get_user() 
  {
    return "";
  }

>>>>>>> jeet
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

<<<<<<< HEAD
  chat_message_queue get_messages()
  {
      return recent_msgs_;
  }

=======
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
    

>>>>>>> jeet
private:
    std::string name;
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
  chat_session(tcp::socket socket, chat_room* room/*, std::string p_name*/)
  : socket_(std::move(socket)),
    room_(room)//, name_(p_name)
  {
<<<<<<< HEAD
      //name = p_name;
=======
    uuid_ = boost::uuids::random_generator()();
>>>>>>> jeet
  }

  void start()
  {
    room_->join(shared_from_this());
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
<<<<<<< HEAD
    std::string name_;
=======
  std::string get_time()
  {
    using namespace boost::posix_time;
    boost::posix_time::ptime time_local = boost::posix_time::second_clock::local_time();
    return to_iso_string(time_local); 
  }
  
  std::string get_crc(const char *data, int length)
  {
    //Got this part of the code from cksum.cpp 
    unsigned int crc;
    
    crc = crc32(0L, Z_NULL, 0);
    
    crc = crc32(crc, (const unsigned char*) data, length);
    return std::to_string(crc);
  }

>>>>>>> jeet
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
            room_->leave(shared_from_this());
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
<<<<<<< HEAD
            room_->deliver(read_msg_);
=======
  	   // std::cout << "received " << read_msg_.body() << std::endl;
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
/*		
	    std::cout << "received " << cmd << std::endl;
	    std::cout << "received " << arg << std::endl;
*/
	    if(cmd == "REQUUID")
	    {
	      chat_message msg;
        std::string body = cmd + "," + to_string(uuid_);
	      std::string data = get_body(body);
	      msg.body_length(std::strlen(data.c_str()));     
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
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
              std::string body = cmd + "," + room_name;
	      std::string data = get_body(body);

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
              std::string body = cmd + "," + names;
	      std::string data = get_body(body);

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
              std::string body = cmd + "," + arg;
	      std::string data = get_body(body);

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
                  std::string body = cmd + "," + name;
   	   	  std::string data = get_body(body);

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
        std::string body = cmd + "," + std::to_string(arg.length());
	      std::string data = get_body(body);
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

              std::string body = cmd + "," + text;
	      std::string data = get_body(body);

	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);
	    }
	    else if(cmd == "REQUSERS")
	    {
	      chat_message msg;
	      std::string users = room_.get_users();
              std::string body = cmd + "," + users;
	      std::string data = get_body(body);

	      msg.body_length(std::strlen(data.c_str()));
	      std::memcpy(msg.body(), data.c_str(), msg.body_length());
	      msg.encode_header();
              deliver(msg);
	    }
	    else
	    {
	      room_.deliver(read_msg_);
	    }
>>>>>>> jeet
            do_read_header();
          }
          else
          {
            room_->leave(shared_from_this());
          }
        });
  }

  std::string get_body(std::string body)
  {
    std::string time = get_time();
    std::string crc = get_crc(body.c_str(), std::strlen(body.c_str()));
    std::string data = crc + "," + time + "," + body;

    return data;
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
            room_->leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room* room_;
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
                   //chat_room room =  *(*iterator);
                   chat_session* session = new chat_session(std::move(socket), *iterator/*, p_name*/);
                    (*iterator)->join((chat_participant_ptr)session);
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
      std::list<std::string> users;
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if(c_name == (*iterator)->get_name())
            {
                return (*iterator)->get_users();
            }
        }
      return users;
  }

  chat_message_queue reqtext(std::string c_name)
  {
      chat_message_queue recent_msgs;
      std::list<chat_room*>::const_iterator iterator;
        for (iterator = chatrooms.begin(); iterator != chatrooms.end(); ++iterator)
        {
            if(c_name == (*iterator)->get_name())
            {
                return (*iterator)->get_messages();
            }
        }
      return recent_msgs;
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
  chat_room* room_;
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
