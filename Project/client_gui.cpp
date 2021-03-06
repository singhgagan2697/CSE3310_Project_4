// This example will demonstrate the integration of
// the boost asio chatserver with a simple FLTK program
//
//
//
#include <assert.h>

#include "io.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Menu_Button.H>

//Screen 1--------------------------------------------------

Fl_Window win1	  	(100,100,650, 300, "Login");
Fl_Output welcome 	(70,30, 510, 50);
Fl_Output welcome2	(130, 90, 470, 30);
Fl_Input nick	    	(170, 150, 300, 20, "Nick:");
Fl_Output nick_disc	(170, 180, 250, 20);
Fl_Button submit  	(225, 220, 200, 30, "    Enter"); 
Fl_Check_Button mod_button(500, 270, 50, 20, "Moderator?");


//Screen 2----------------------------------------------------

Fl_Window win   	(900, 405, "UberChat");
Fl_Output welcome_nick	(10, 10, 150, 20);
Fl_Button mod   	(690, 10, 80, 20, "Moderator");
Fl_Output rooms_title 	(10, 40, 150, 20);
Fl_Button add_room 	(140, 380, 18, 13, "+");
Fl_Output chat_title	(165, 40, 570, 20);
Fl_Button delet 	(675, 42, 58, 16, "Delete");
Fl_Input input1 	(165, 375, 520, 20);
Fl_Button send_b 	(685, 375, 50, 20, "Send");
Fl_Output nicks_title 	(740, 40, 150, 20);

Fl_Button quit  (800, 10, 40,20,"Quit");
Fl_Button clear (850, 10, 40,20,"Clear");

Fl_Text_Buffer *buff = new Fl_Text_Buffer ();
<<<<<<< HEAD
=======
Fl_Text_Buffer *rooms_buff = new Fl_Text_Buffer();
>>>>>>> jeet
Fl_Text_Display *rooms = new Fl_Text_Display(10, 60, 150, 335);
Fl_Text_Display *disp = new Fl_Text_Display (165, 60, 570, 315);
Fl_Text_Display *nicks = new Fl_Text_Display(740, 60, 150, 335);


//Screen 3 -----------------------------------

Fl_Window win3		(300, 200, "Add a Room");
Fl_Input room_name	(100, 20, 190, 50, "Chat Room Name: ");
Fl_Button create	(30, 100, 100, 50, "Create");
Fl_Button cancel	(170, 100, 100, 50, "Cancel");


//Screen 4-----------------------------------
Fl_Window win4		(350, 250, "Moderator");
Fl_Menu_Button select_room(10, 10, 330, 30, "Room: ");
Fl_Menu_Button select_nick(10,50, 330, 30, "Nick: ");
Fl_Menu_Button select_action(10, 90, 330, 30, "Action: ");
Fl_Button do_action	(130, 200, 100, 40, "Submit");
Fl_Button cancel_action (240, 200, 100, 40, "Cancel");


// boost asio instances

chat_client *c = NULL;
std::thread *t = NULL;


static void cb_recv ( std::string S )
{
  // Note, this is an async callback from the perspective
  // of Fltk..
  //
  // high chance of a lock needed here if certain fltk calls
  // are made.  (like show() .... )
  
  std::vector<std::string> tokens = c->decode_msg(S);
  if(tokens.size() > 2)
  {
    std::cout << "received command --- " << tokens.at(2) << std::endl;
    if(tokens.at(2).compare("REQCHATROOM") == 0)
    {
      if(tokens.size() == 4)
      {
        rooms_title.value(tokens.at(3).c_str());
      }
    }
    else if(tokens.at(2).compare("REQUUID") == 0)
    {
      if(tokens.size() == 4)
      {
        c->set_uuid(tokens.at(3));
      }
    }
    else if(tokens.at(2).compare("RECHATROOMS") == 0)
    {
      if(tokens.size() == 4)
      {
        rooms_buff->append((tokens.at(3)).c_str());
      }
    }
    else if(tokens.at(2).compare("SENDTEXT") == 0)
    {
      if(tokens.size() == 4)
      {
        std::string T = c->get_name() + ": " +tokens.at(2)+ tokens.at(3) + '\n' + '\0';
        if (buff)
        {
          buff->append ( T.c_str () );
        }
        if (disp)
        {
          disp->show ();
        }
      }
    }
  }
  win.show ();
}

//SCREEN 4 -----------------------------------------------

//Callback
static void cb_do_action(){
  win4.hide();
}

static void cb_cancel_action(){
  win4.hide();
}

//Begin
void begin_mod_action(){
  win4.begin();
    win4.color(FL_WHITE);
    win4.add(select_room);
    select_room.color(FL_WHITE);
    win4.add(select_nick);
    select_nick.color(FL_WHITE);
    win4.add(select_action);
    select_action.color(FL_WHITE);
    win4.add(do_action);
    do_action.callback((Fl_Callback*)cb_do_action);
    win4.add(cancel_action);
    cancel_action.callback((Fl_Callback*)cb_cancel_action);
  win4.end();
  win4.show();
}


//SCREEN 3----------------------------------------------------

//Calback
void cb_create(){
  win3.hide();
}

void cb_cancel(){
  room_name.value("");
  win3.hide();
}

//Begin
void begin_add_room(){
  win3.begin();
    win3.color(FL_WHITE);
    win3.add(room_name);
    room_name.callback((Fl_Callback *)cb_create, (void *) "room");
    room_name.when(FL_WHEN_ENTER_KEY);
    win3.add(create);
    create.callback((Fl_Callback*)cb_create);
    win3.add(cancel);
    cancel.callback((Fl_Callback*)cb_cancel);
  win3.end();
  win3.show();
}


//SCREEN 2 -------------------------------------------------------

//Callbacks
static void cb_delet(){

}


static void cb_add_room(){
  begin_add_room();
}

static void cb_mod(){
  begin_mod_action();
}

static void cb_clear ()
{
   if (buff)
   {
     buff->remove (0,  buff->length () );
   }
   // may need to call show() ?
}

static void cb_quit ( )
{
  // this is where we exit to the operating system
  // any clean up needs to happen here
  //
  if (c)
  {
    c->close();
  }
  if (t)
  {
     t->join();
  }

  exit (0);
}


static void cb_input1 (Fl_Input*, void * userdata) 
{
<<<<<<< HEAD
  chat_message msg;
  msg.body_length(std::strlen( ( const char *) input1.value ()) + 1);
  // ensure it is null terminated
  std::memset (msg.body(), 0, msg.body_length());
  // copy over the payload
  std::memcpy (msg.body(), ( const char *) input1.value (), msg.body_length()-1);
  msg.encode_header();
  std::cout << "sent " << msg.body() << std::endl;
  c->write(msg);
=======
  c->sendtext((const char*) input1.value());
  std::cout << "sent " << (const char*) input1.value() << std::endl;
>>>>>>> jeet
  input1.value("");
}


//Begin
void beginChat(const char *nick_name){
  win.begin();
    win.color(FL_WHITE);
    welcome_nick.value(nick_name);	//name of nick on top left
    win.add(welcome_nick);
    win.add(mod);			//moderator
    mod.callback((Fl_Callback *) cb_mod);

//List of Rooms--------------------------------

    rooms_title.value("Rooms Available");
    rooms_title.box(FL_BORDER_BOX);
    win.add(rooms_title);
    rooms->box(FL_BORDER_BOX);
<<<<<<< HEAD
=======
    rooms->buffer(rooms_buff);
>>>>>>> jeet
    win.add(rooms);
    win.add(add_room);
    add_room.callback((Fl_Callback*)cb_add_room);

//Chat box -----------------------------------

    chat_title.value("Current Chat");
    chat_title.box(FL_BORDER_BOX);
    win.add(chat_title);
    win.add(delet);
    delet.color(FL_RED);
    delet.labelsize(13);
    delet.callback((Fl_Callback*)cb_delet);
    input1.box(FL_BORDER_BOX);
    win.add (input1);
    input1.callback ((Fl_Callback*)cb_input1,( void *) "Enter next:");
    input1.when ( FL_WHEN_ENTER_KEY );
    send_b.box(FL_BORDER_BOX);
    send_b.color(FL_WHITE);
    win.add(send_b);
    send_b.callback ((Fl_Callback*)cb_input1, (void*) "Enter next:");
    disp->buffer(buff);
    disp->box(FL_BORDER_BOX);
    win.add(disp);

//List of nicks

    nicks_title.value("Nicks:");
    nicks_title.box(FL_BORDER_BOX);
    win.add(nicks_title);
    nicks->box(FL_BORDER_BOX);
    win.add(nicks);
    clear.callback (( Fl_Callback*) cb_clear );
    win.add(clear);
    quit.callback (( Fl_Callback*) cb_quit );
    win.add (quit);
  win.end ();
  win.show ();
}


//SCREEN 1 ----------------------------------------------------

//Callbacks
static void cb_submit(){
  const char * nick_name = nick.value();
<<<<<<< HEAD
  beginChat(nick_name);
  win1.hide();
}

//Begin
void beginLogin(){
  win1.color(FL_WHITE);
  win1.begin();
    welcome.value("  Welcome To UberChat");
    welcome.textsize(40);
    welcome.box(FL_BORDER_BOX);
    win1.add(welcome);
    welcome2.value("The new way to communicate");
    welcome2.textsize(20);
    welcome2.box(FL_NO_BOX);
    win1.add(welcome2);
    win1.add(nick);
    nick_disc.value("Enter a nick name to get started");
    nick_disc.box(FL_NO_BOX);
    win1.add(nick_disc);
    nick.callback((Fl_Callback *)cb_submit, (void*) "Enter nick:");
    nick.when(FL_WHEN_ENTER_KEY);
    win1.add(submit);
    submit.callback((Fl_Callback *)cb_submit);
    win1.add(mod_button);
  win1.end();
  win1.show();
}



=======
  c->nick(std::string(nick_name, std::strlen(nick_name)));
  c->set_name(nick_name);
  c->requuid();
  beginChat(nick_name);
  win1.hide();
}

//Begin
void beginLogin(){
  win1.color(FL_WHITE);
  win1.begin();
    welcome.value("  Welcome To UberChat");
    welcome.textsize(40);
    welcome.box(FL_BORDER_BOX);
    win1.add(welcome);
    welcome2.value("The new way to communicate");
    welcome2.textsize(20);
    welcome2.box(FL_NO_BOX);
    win1.add(welcome2);
    win1.add(nick);
    nick_disc.value("Enter a nick name to get started");
    nick_disc.box(FL_NO_BOX);
    win1.add(nick_disc);
    nick.callback((Fl_Callback *)cb_submit, (void*) "Enter nick:");
    nick.when(FL_WHEN_ENTER_KEY);
    win1.add(submit);
    submit.callback((Fl_Callback *)cb_submit);
    win1.add(mod_button);
  win1.end();
  win1.show();
}



>>>>>>> jeet
int main ( int argc, char **argv) 
{
  beginLogin();
  
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
    c = new chat_client (io_service, endpoint_iterator, &cb_recv);
    t = new std::thread ([&io_service](){ io_service.run(); });
	  


    // goes here, never to return.....
    return Fl::run ();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  // never gets here
  return 0;
}
