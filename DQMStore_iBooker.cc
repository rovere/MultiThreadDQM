// g++ DQMStore_iBooker.cc -Wall --std=c++0x -o DQMStore_iBooker.x -lpthread `root-config --cflags --libs`

#include <TH1F.h>

#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>
#include <thread>
#include <set>
#include <vector>

const int NUM_MODULES = 20;
const int NUM_FILLS = 10000000;

class MonitorElement {
 public:
  MonitorElement(const std::string *dir,
                 const std::string *name,
                 uint32_t run,
                 uint32_t streamId,
                 uint32_t moduleId)
      :run_(run),
       streamId_(streamId),
       moduleId_(moduleId),
       histo_(nullptr),
       dirname_(dir),
       name_(name) {
    std::cout << "MonitorElement() "
              << *dirname_ << "/"
              << *name_ << std::endl;
  }
  // for ordering into set.
  bool operator< (MonitorElement const & rhs) const {
    if (run_ == rhs.run()) {
      if (streamId_ == rhs.streamId()) {
        if (*dirname_ == rhs.dir()) {
          return *name_ < rhs.name();
        }
        return *dirname_ < rhs.dir();
      }
      return streamId_ < rhs.streamId();
    }
    return run_ < rhs.run();
    assert(false);
  }

  uint32_t run() const {return run_;}
  uint32_t streamId() const {return streamId_;}
  uint32_t moduleId() const {return moduleId_;}
  std::string dir() const {return *dirname_;}
  std::string name() const {return *name_;}
  double mean() const {return histo_->GetMean();}
  double rms() const {return histo_->GetRMS();}
  double entries() const {return histo_->GetEntries();}
  MonitorElement * initialize(TH1 * obj) {
    if (obj)
      histo_ = obj;
    return this;
  }
  void Fill(double val) {
    if (histo_)
      histo_->Fill(val);
  }

 private:
  uint32_t run_;
  uint32_t streamId_;
  uint32_t moduleId_;
  TH1 * histo_;
  const std::string * dirname_;
  const std::string * name_;
};  // MonitorElement


class DQMStore {

 public:
  class IBooker {

  public:
    friend class DQMStore;
    MonitorElement * book1d(std::string);
    void cd(std::string dir);

  private:
    explicit IBooker(DQMStore * store):dqmstore_(0) {
      assert(store);
      dqmstore_ = store;
    }
    IBooker();
    IBooker(const IBooker&);

    DQMStore * dqmstore_;
  };  // IBooker

  static DQMStore * getInstance() {
    if (!instance_)
      return (instance_ = new DQMStore());
    return instance_;
  }

  template <typename iFunc>
  void bookTransition(iFunc f,
                      uint32_t run,
                      uint32_t streamId,
                      uint32_t moduleId) {
    std::lock_guard<std::mutex> guard(book_mutex_);
    run_ = run;
    streamId_ = streamId;
    moduleId_ = moduleId;
    f(*ibooker_);
  }

  void dump(bool printStat = false) {
    unsigned int n = 0;
    for (auto item : data_) {
      std::cout << ++n << " --> "
                << "Run: " << item.run()
                << " streamId: " << item.streamId()
                << " moduleId: " << item.moduleId()
                << " " << item.dir()
                << "/" << item.name();
      if (printStat)
        std::cout << " " << item.mean() << " "
                  << item.rms() << " "
                  << std::setprecision(9) << item.entries();
      std::cout << std::endl;
    }
  }

  void cd(std::string dir) {
    if (!dirs_.count(dir))
      dirs_.insert(dir);
    pwd_ = dir;
  }

  uint32_t run() const {return run_;}
  uint32_t streamId() const {return streamId_;}
  uint32_t moduleId() const {return moduleId_;}

 private:
  DQMStore(void) {
    if (!ibooker_)
      ibooker_ = new DQMStore::IBooker(this);
  }
  DQMStore(const DQMStore&);
  MonitorElement * book1d(std::string);
  static DQMStore * instance_;
  IBooker * ibooker_;
  std::mutex book_mutex_;
  std::set<MonitorElement> data_;
  std::set<std::string> dirs_;
  std::set<std::string> objs_;
  std::string pwd_;
  uint32_t run_;
  uint32_t streamId_;
  uint32_t moduleId_;

  const std::string * pwd() const  {
    assert(dirs_.count(pwd_));
    return &(*dirs_.find(pwd_));
  };

  const std::string * me_name(std::string name) {
    if (!objs_.count(name))
      return &(*objs_.insert(name).first);
    return &(*objs_.find(name));
  }
};  // DQMStore

class Module {
 public:
  Module() {}
  void bookHistograms(std::string dir,
                      uint32_t run,
                      uint32_t streamId,
                      uint32_t moduleId);
  void fillHistograms(double val);
 private:
  std::vector<MonitorElement*> mes_;
};  // Module

// Static Variables
DQMStore * DQMStore::instance_ = 0;

MonitorElement * DQMStore::book1d(std::string name) {
  MonitorElement me(pwd(), me_name(name), run(), streamId(), moduleId());
  TH1F * tmp = new TH1F(name.c_str(), name.c_str(), 1000, 0., 1000.);
  me.initialize(tmp);
  return &(const_cast<MonitorElement &>(*(data_.insert(me).first)));
}

void DQMStore::IBooker::cd(std::string dir) {
  dqmstore_->cd(dir);
}

MonitorElement * DQMStore::IBooker::book1d(std::string name) {
  return dqmstore_->book1d(name);
}

void Module::bookHistograms(std::string dir,
                            uint32_t run,
                            uint32_t streamId,
                            uint32_t moduleId) {
  DQMStore * store = DQMStore::getInstance();
  store->bookTransition([&](DQMStore::IBooker & b) {
      b.cd(dir);
      mes_.push_back(b.book1d(std::string("pippo")));
      mes_.push_back(b.book1d(std::string("pluto")));
      mes_.push_back(b.book1d(std::string("paperino")));
    }, run, streamId, moduleId);
};

void Module::fillHistograms(double val) {
  for (auto & histo : mes_)
    for (int j = 0; j < NUM_FILLS; ++j)
      histo->Fill(val);
};

void another_booking(std::string dir,
                     uint32_t run,
                     uint32_t streamId,
                     uint32_t moduleId) {
  DQMStore * store = DQMStore::getInstance();
  store->bookTransition([&](DQMStore::IBooker & b) {
      b.cd(dir);
      b.book1d(std::string("pippo"));
      b.book1d(std::string("pluto"));
      b.book1d(std::string("paperino"));
    }, run, streamId, moduleId);
};

template<typename A>
void join_and_dump(A& a, DQMStore * store, bool print_stat = false) {
  for (unsigned int i = 0; i < a.size(); ++i)
    a[i].join();
  store->dump(print_stat);
}

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

  for (int d = 0; d < NUM_MODULES; ++d) {
    snprintf(folder_name, sizeof(folder_name), "%s_%d", "Folder", d);
    Module * tmp = new Module();
    modules.push_back(tmp);
    v_booking.push_back(std::thread(&Module::bookHistograms,
                                    &(*tmp),
                                    std::string(folder_name),
                                    d < NUM_MODULES/2 ? d : 10*d,
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
                                    j*10.));
  }

  join_and_dump(v_filling, store, true);

  return 0;
}

/**
 Local Variables:
 show-trailing-whitespace: t
 truncate-lines: t
 End:
*/
