#include "MainWindow.h"

MainWindow::MainWindow()
    : button_("Hello World")
{
  set_border_width(10);

  button_.signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::on_button_clicked));

  add(button_);

  button_.show();
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_button_clicked()
{
}