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


Fl_Window win   (350, 350, "SimpleChat");
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


int main ( int argc, char **argv) 
{
  win.begin ();
    win.add (input1);
    input1.callback ((Fl_Callback*)cb_input1,( void *) "Enter next:");
    input1.when ( FL_WHEN_ENTER_KEY );
    quit.callback (( Fl_Callback*) cb_quit );
    clear.callback (( Fl_Callback*) cb_clear );
    win.add (quit);
    disp->buffer(buff);
  win.end ();
  win.show ();
	
	
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
	  
	  	begin_screen_2();

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
