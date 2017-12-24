#include <gtkmm/application.h>
#include "MainWindow.h"

int main (int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "ru.ivsoft.vds");

  MainWindow mainwnd;

  //Shows the window and returns when it is closed.
  return app->run(mainwnd);
}
