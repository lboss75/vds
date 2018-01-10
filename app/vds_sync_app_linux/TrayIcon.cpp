//
// Created by vadim on 10.01.18.
//

#include "TrayIcon.h"

TrayIcon::TrayIcon() {
  set(Gtk::Stock::OK);

  signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::on_statusicon_activated));
  signal_popup_menu().connect(sigc::mem_fun(*this, &TrayIcon::on_statusicon_popup));

  set_visible(true);

  printf("Statusicon created\n");
}

void TrayIcon::on_statusicon_popup(guint button, guint activate_time) {
  printf("popup!");
}

void TrayIcon::on_statusicon_activated() {
  printf("active!");
}
