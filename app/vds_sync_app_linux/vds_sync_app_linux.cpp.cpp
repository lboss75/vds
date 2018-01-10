#include <gtkmm/application.h>
#include "MainWindow.h"
#include "TrayIcon.h"

int main (int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "ru.ivsoft.vds");

//  auto try_icon = Gtk::StatusIcon::create_from_file("/home/vadim/projects/vds.git/app/vds_sync_app_linux/StatusIcon.png");
//  try_icon->set_tooltip_text("hello!");
//  try_icon->set_visible(true);
//
//  MainWindow mainwnd;
//  mainwnd.hide();
//
//  return app->run(mainwnd);

  app->register_application();
  auto notification = Gio::Notification::create("Hello world");
  notification->set_body("This is an example notification.");

  auto icon = Gio::ThemedIcon::create("dialog-information");
  notification->set_icon (icon);
  app->send_notification(notification);

  return 0;
}
