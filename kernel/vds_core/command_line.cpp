/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include <iostream>
#include <set>
#include "command_line.h"
///////////////////////////////////////
//              vds_command_item
///////////////////////////////////////
vds::command_line_item::command_line_item(
  const std::string& name,
  const std::string& description)
: name_(name), description_(description)
{
}

vds::command_line_item::~command_line_item()
{
}
///////////////////////////////////////
//              vds_command_value
///////////////////////////////////////
vds::command_line_value::command_line_value(
  const std::string& sort_switch,
  const std::string& long_switch,
  const std::string& name,
  const std::string& description)
: command_line_item(name, description),
sort_switch_(sort_switch), long_switch_(long_switch)
{
}

const std::string& vds::command_line_value::value() const
{
  return this->value_;
}

int vds::command_line_value::try_parse(
  int argc,
  const char** argv)
{
  if(argc < 2) {
    return -1;
  }
  
  if(
    (this->sort_switch_.empty() || argv[0] != "-" + this->sort_switch_)
    && argv[0] != "--" + this->long_switch_) {
    return -1;
  }
  
  if(argv[1][0] == '-'){
    throw command_line_item_error(
      "Value is required",
      this->name());
  }
  
  if(!this->value_.empty()){
    throw command_line_item_error(
      "Value already has been specified",
      this->name());
  }
  
  this->value_ = argv[1];
  return 2;
}

void vds::command_line_value::print_usage(
  std::map<std::string, const vds::command_line_item* >& items) const
{
  auto switch_text = "-" + this->sort_switch_;
  std::cout << switch_text << " <" << this->name() << ">";
  
  if(items.end() == items.find(switch_text)){
    items[switch_text] = this;
  }
}

void vds::command_line_value::print_help() const
{
  if(this->sort_switch_.empty()){
    std::cout << "\t";
  }
  else {
    std::cout << "-" << this->sort_switch_;
  }

  if(this->long_switch_.empty()){
    std::cout << "\t";
  }
  else {
    if(!this->sort_switch_.empty()){
      std::cout << ",\t";
    }
    std::cout << "--" << this->long_switch_;
  }
  
  std::cout 
  << " <"
  << this->name()
  << "> "
  << this->description()
  << "\n";
}

void vds::command_line_value::clear()
{
  this->value_.clear();
}
///////////////////////////////////////
//              vds_command_values
///////////////////////////////////////
vds::command_line_values::command_line_values(
  const std::string& sort_switch,
  const std::string& long_switch,
  const std::string& name,
  const std::string& description)
: command_line_item(name, description),
sort_switch_(sort_switch), long_switch_(long_switch)
{
}

const std::list<std::string>& vds::command_line_values::values() const
{
  return this->values_;
}

int vds::command_line_values::try_parse(
  int argc,
  const char** argv)
{
  if(argc < 2) {
    return -1;
  }
  
  if(
    argv[0] != "-" + this->sort_switch_
    && argv[0] != "--" + this->long_switch_) {
    return -1;
  }
  
  if(argv[1][0] == '-'){
    throw command_line_item_error(
      "Value is required",
      this->name());
  }
  
  this->values_.push_back(argv[1]);
  return 2;
}

void vds::command_line_values::print_usage(std::map< std::string, const vds::command_line_item* >& items) const
{
  auto switch_text = "-" + this->sort_switch_;
  std::cout << switch_text << " <" << this->name() << ">";
  
  if(items.end() == items.find(switch_text)){
    items[switch_text] = this;
  }
}

void vds::command_line_values::print_help() const
{
  if(this->sort_switch_.empty()){
    std::cout << "\t";
  }
  else {
    std::cout << "-" << this->sort_switch_;
  }

  if(this->long_switch_.empty()){
    std::cout << "\t";
  }
  else {
    if(!this->sort_switch_.empty()){
      std::cout << ",\t";
    }
    std::cout << "--" << this->long_switch_ << "\t";
  }
  
  std::cout << this->name() << " " << this->description() << "\n";
}

void vds::command_line_values::clear()
{
  this->values_.clear();
}
///////////////////////////////////////
//              vds_command_switch
///////////////////////////////////////
vds::command_line_switch::command_line_switch(
  const std::string& sort_switch,
  const std::string& long_switch,
  const std::string& name,
  const std::string& description)
: command_line_item(name, description),
sort_switch_(sort_switch), long_switch_(long_switch)
{
}

bool vds::command_line_switch::value() const
{
  return this->value_;
}

int vds::command_line_switch::try_parse(
  int argc,
  const char** argv)
{
  if(argc < 1) {
    return -1;
  }
  
  if(
    (this->sort_switch_.empty() || argv[0] != "-" + this->sort_switch_)
    && argv[0] != "--" + this->long_switch_) {
    return -1;
  }
  
  if(this->value_){
    throw command_line_item_error(
      "Switch already has been specified",
      this->name());
  }
  
  this->value_ = true;
  return 1;
}

void vds::command_line_switch::print_usage(std::map< std::string, const vds::command_line_item* >& items) const
{
  auto switch_text = this->sort_switch_.empty() ? ("--" + this->long_switch_) : ("-" + this->sort_switch_);
  std::cout << switch_text;
  
  if(items.end() == items.find(switch_text)){
    items[switch_text] = this;
  }
}

void vds::command_line_switch::print_help() const
{
  if(this->sort_switch_.empty()){
    std::cout << "\t";
  }
  else {
    std::cout << "-" << this->sort_switch_;
  }

  if(this->long_switch_.empty()){
    std::cout << "\t";
  }
  else {
    if(!this->sort_switch_.empty()){
      std::cout << ",\t";
    }
    std::cout << "--" << this->long_switch_;
  }
  
  std::cout << this->description() << "\n";
}

void vds::command_line_switch::clear()
{
  this->value_ = false;
}
///////////////////////////////////////
//              vds_command_set
///////////////////////////////////////
vds::command_line_set::command_line_set(
  const std::string& name,
  const std::string& description,
  const std::string& command,
  const std::string& categoty)
: name_(name), description_(description),
command_(command), categoty_(categoty)
{
}

void vds::command_line_set::optional(
  vds::command_line_item& item)
{
  this->optional_.push_back(&item);
}

void vds::command_line_set::required(
  vds::command_line_item& item)
{
  this->required_.push_back(&item);
}

int vds::command_line_set::try_parse(
  int argc,
  const char** argv,
  std::string & last_error)
{  
  if(!this->categoty_.empty()){
    if(argc < 1 || argv[0] != this->categoty_){
      return argc + 1;
    }
    
    ++argv;
    --argc;
  }
  
  if(!this->command_.empty()){
    if(argc < 1 || argv[0] != this->command_){
      return argc + 1;
    }
    
    ++argv;
    --argc;
  }
  
  for(auto p : this->required_){
    p->clear();
  }
  
  for(auto p : this->optional_){
    p->clear();
  }
  
  std::set<command_line_item *> required;
  
  while(argc > 0) {
    bool bgood = false;
    for(auto p : this->required_){
      auto res = p->try_parse(argc, argv);
      if(res > 0) {
        argc -= res;
        argv += res;
        required.insert(p);
        bgood = true;
        break;
      }
    }
    
    if(bgood){
      continue;
    }
    
    for(auto p : this->optional_){
      auto res = p->try_parse(argc, argv);
      if(res > 0) {
        argc -= res;
        argv += res;
        bgood = true;
        break;
      }
    }
    
    if(!bgood){
      if(argc <= 0){
        throw std::runtime_error("Logic error 28");
      }
      
      last_error = std::string(argv[0]) + " is unexpected";
      return argc;
    }
  }
  
  if(required.size() == this->required_.size()){
    return 0;
  }
  
  for(auto p : this->required_){
    if(required.end() == required.find(p)) {
      last_error = p->name() + " is required";
      return argc;
    }
  }
  
  throw std::runtime_error("error");
}

void vds::command_line_set::print_usage(std::map<std::string, const command_line_item *> & items) const
{
  if(!this->categoty_.empty()){
    std::cout << this->categoty_;
    std::cout << " ";
  }
  
  if(!this->command_.empty()){
    std::cout << this->command_;
    std::cout << " ";
  }
  
  for(auto p : this->required_){
    p->print_usage(items);
    std::cout << " ";
  }
  
  for(auto p : this->optional_){
    std::cout << "[";
    p->print_usage(items);
    std::cout << "] ";
  }
  
  std::cout << " - " << this->description_;
}
//////////////////////////////////////////////////
//
//////////////////////////////////////////////////
vds::command_line::command_line(
  const std::string& name,
  const std::string& description,
  const std::string& version)
: name_(name), description_(description),
version_(version)
{

}

const vds::command_line_set* vds::command_line::parse(
  int argc,
  const char** argv
) const
{
  --argc;
  ++argv;
  
  const vds::command_line_set* best = nullptr;
  int bestleft = 0;
  std::string best_error;
  
  for(auto p : this->command_sets_){
    std::string last_error;
    auto left = p->try_parse(argc, argv, last_error);
    if(0 == left) {
      return p;
    }
    if(nullptr == best || bestleft > left) {
      best = p;
      bestleft = left;
      best_error = last_error;
    }
  }
  
  if(nullptr == best || best_error.empty()){
    return nullptr;
  }
  
  throw std::runtime_error(best_error);
}

void vds::command_line::add_command_set(vds::command_line_set& item)
{
  this->command_sets_.push_back(&item);
}

void vds::command_line::show_help(
  const std::string & app_name
)
{
 std::cout
  << this->name_
  << " " << this->version_
  << " " << this ->description_
  << "\n";
  
  std::map<std::string, const command_line_item *> items;
  bool first = true;
  for(auto p : this->command_sets_){
    if(first) {
      std::cout << "Usage: ";
      first = false;
    }
    else {
      std::cout << "or: ";
    }
    
    std::cout << app_name << " ";
    p->print_usage(items);
    std::cout << "\n";
  }
  
  std::cout << "Options:\n";
  
  for(auto & p : items){
    p.second->print_help();
  }
}

vds::command_line_item_error::command_line_item_error(
  const std::string& message,
  const std::string& item_name)
: std::runtime_error(message)
{

}
