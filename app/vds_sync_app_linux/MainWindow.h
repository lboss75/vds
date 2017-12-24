#ifndef VDS_MAINWINDOW_H
#define VDS_MAINWINDOW_H

#include <gtkmm/button.h>
#include <gtkmm/window.h>

class MainWindow  : public Gtk::Window
{

public:
  MainWindow();
  virtual ~MainWindow();

protected:
  void on_button_clicked();

  Gtk::Button button_;
};


#endif //VDS_MAINWINDOW_H
