//
// Created by vadim on 02.01.18.
//
#include "stdafx.h"

#ifdef _DEBUG
#include "leak_detect.h"
#include <mutex>

static std::mutex objects_mutex_;
static std::set<vds::ileak_detect_helper *> objects_;

void vds::leak_detect::add_object(vds::ileak_detect_helper *helper) {
  std::unique_lock<std::mutex> lock(objects_mutex_);
  objects_.emplace(helper);
}

void vds::leak_detect::remove_object(vds::ileak_detect_helper *helper) {
  std::unique_lock<std::mutex> lock(objects_mutex_);
  objects_.erase(helper);
}


std::string vds::leak_detect_resolver::resolve() {
  for(auto p : objects_){
    this->unprocessed_.emplace(p);
  }

  while(!this->unprocessed_.empty()){
    auto p = this->unprocessed_.begin();
    auto item = *p;
    this->unprocessed_.erase(p);

    this->references_[item] = target_info();

    leak_detect_collector collector(this, item);
    item->dump_leaks(&collector);
  }

  for(auto root : this->root_objects_){
    this->mark_as_root(root);
  }

  std::set<std::string> lines;

  for(auto p : this->references_){
    if(!p.second.from_root_){//Leak

      if(p.second.targets_.empty()){
        std::string path = p.first->name();
        if(lines.end() == lines.find(path)){
          lines.emplace(path);
        }
      }
      else {
        std::set<ileak_detect_helper *> processed;
        processed.emplace(p.first);

        for (auto ref : p.second.targets_) {
          this->process_leak(lines, processed, ref);
        }
      }
    }
  }

  std::string result;
  for(auto p : lines){
    result += p;
    result += '\n';
  }

  return result;
}

void vds::leak_detect_resolver::mark_as_root(vds::ileak_detect_helper *obj) {
  auto p = this->references_.find(obj);
  if(this->references_.end() != p && !p->second.from_root_){
    p->second.from_root_ = true;

    for(auto target : p->second.targets_){
      this->mark_as_root(target);
    }
  }
}

void vds::leak_detect_resolver::process_leak(
    std::set<std::string> &result,
    const std::set<ileak_detect_helper *> &processed,
    ileak_detect_helper *target) {
  auto p = processed.find(target);
  if(processed.end() == p){
    std::set<ileak_detect_helper *> new_processed(processed);
    new_processed.emplace(target);

    auto refs = this->references_.find(target);
    if(!refs->second.from_root_) {
      for (auto p : refs->second.targets_) {
        this->process_leak(result, new_processed, p);
      }
    }
  }
  else {
    std::string path;
    while(processed.end() != p){
      path += (*p)->name();
      path += '/';
      ++p;
    }

    if(result.end() == result.find(path)){
      result.emplace(path);
    }
  }
}
#endif