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

Fl_Window win1	  (100,100,650, 300, "Login");
Fl_Output welcome (70,30, 510, 50);
Fl_Output welcome2(130, 90, 470, 30);
Fl_Input nick	    (170, 150, 300, 20, "Nick:");
Fl_Output nick_disc(170, 180, 250, 20);
Fl_Button submit  (225, 220, 200, 30, "    Enter"); 
Fl_Check_Button mod_button(500, 270, 50, 20, "Moderator?");


Fl_Window win   (750, 400, "UberChat");
Fl_Button mod   (650, 10, 80, 20, "Moderator");
Fl_Output rooms (10, 40, 100, 20);
Fl_Input input1 (30, 10, 180, 20, "In: ");
Fl_Button quit  (30, 275, 50,20,"Quit");
Fl_Button clear (80, 275, 50,20,"Clear");

Fl_Text_Buffer *buff = new Fl_Text_Buffer ();
Fl_Text_Display *disp = new Fl_Text_Display (30,70,300,200,"chat");


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
  std::string T = S + '\n' + '\0';
  if (buff)
  {
    buff->append ( T.c_str () );
  }
  if (disp)
  {
    disp->show ();
  }

  win.show ();
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
  chat_message msg;
  msg.body_length(std::strlen( ( const char *) input1.value ()) + 1);
  // ensure it is null terminated
  std::memset (msg.body(), 0, msg.body_length());
  // copy over the payload
  std::memcpy (msg.body(), ( const char *) input1.value (), msg.body_length()-1);
  msg.encode_header();
  std::cout << "sent " << msg.body() << std::endl;
  c->write(msg);
}

void beginChat(std::string nick_name){
  win.begin();
    win.color(FL_WHITE);
    win.add(mod);
    rooms.value("Rooms");
    win.add(rooms);
    win.add (input1);
    input1.callback ((Fl_Callback*)cb_input1,( void *) "Enter next:");
    input1.when ( FL_WHEN_ENTER_KEY );
    quit.callback (( Fl_Callback*) cb_quit );
    clear.callback (( Fl_Callback*) cb_clear );
    win.add (quit);
    disp->buffer(buff);
    
  win.end ();
  win.show ();
}

static void cb_nick(){ 
}

static void cb_submit(){
  std::string nick_name = nick.value();
  beginChat(nick_name);
  win1.hide();
}

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
    nick.callback((Fl_Callback *)cb_nick, (void*) "Enter nick:");
    nick.when(FL_WHEN_ENTER_KEY);
    win1.add(submit);
    submit.callback((Fl_Callback *)cb_submit);
    win1.add(mod_button);
  win1.end();
  win1.show();
}



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
