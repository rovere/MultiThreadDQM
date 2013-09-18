// g++-4.7 toy_test.cc DQMStore.cc MonitorElement.cc Module.cc -Wall --std=c++0x -o toy_test.x -lpthread -I./ `root-config --cflags --libs`

#include <DQMStore.h>
#include <MonitorElement.h>
#include <Module.h>
#include <Utils.h>

#include <iostream>
#include <iomanip>

const int NUM_MODULES = 20;

// Static Variables
DQMStore * DQMStore::instance_ = 0;

int main(int argc, char * argv[]) {
  MonitorElement * me;
  DQMStore * store = DQMStore::getInstance();
  // Prevent ROOT from globally tracing out histograms.
  TH1::AddDirectory(false);
  assert(store);

  char folder_name[1000];
  std::vector<std::thread> v_booking;
  std::vector<std::thread> v_filling;
  // Better use pointers to Module since they are "immutable" over the
  // copy operation done by push_back, so that we do not have to
  // follow closely the real object location in memory. Using Modules
  // directly would make the following code simply wrong and a more
  // complex one should be written.
  std::vector<Module*> modules;
  v_booking.reserve(NUM_MODULES);
  v_filling.reserve(NUM_MODULES);
  modules.reserve(NUM_MODULES);

  for (int d = 1; d <= NUM_MODULES; ++d) {
    snprintf(folder_name, sizeof(folder_name), "%s_%d", "Folder", 10);
    Module * tmp = new Module();
    modules.push_back(tmp);
    v_booking.push_back(std::thread(&Module::bookHistograms,
                                    &(*tmp),
                                    std::string(folder_name),
                                    10,  // d < NUM_MODULES/2 ? d : 10*d,
                                    d, d));
  }
  store->bookTransition([&](DQMStore::IBooker & b) {
      b.cd(std::string("main"));
      me = b.book1d(std::string("foo_bar"));
    }, 1, 100, 100);
  assert(me);
  std::cout << me->dir() << "/" << me->name() << std::endl;

  // Needed to be sure that booking has finished!
  join_and_dump(v_booking, store);

  for (int j = 0; j < NUM_MODULES; ++j) {
    v_filling.push_back(std::thread(&Module::fillHistograms,
                                    &(*modules.at(j)),
                                    (j+1)*10.));
  }

  join_and_dump(v_filling, store, true);

  for (int j = 1; j <= NUM_MODULES; ++j)
    store->mergeAndResetMEs(10, j, j);

  return 0;
}

/**
 Local Variables:
 show-trailing-whitespace: t
 truncate-lines: t
 End:
*/
