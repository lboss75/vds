#ifndef VDS_TRAYICON_H
#define VDS_TRAYICON_H

#include <gtkmm.h>

class TrayIcon : public Gtk::StatusIcon {
public:
  TrayIcon();

private:
  void on_statusicon_popup(guint button, guint activate_time);
  void on_statusicon_activated();
};


#endif //VDS_TRAYICON_H
