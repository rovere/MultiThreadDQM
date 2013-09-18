#ifndef DQMStore_H
#define DQMStore_H

#include <string>
#include <mutex>
#include <thread>
#include <set>
#include <vector>
#include <cassert>

class MonitorElement;

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

  void dump(bool printStat = false);
  void cd(std::string dir);
  uint32_t run() const {return run_;}
  uint32_t streamId() const {return streamId_;}
  uint32_t moduleId() const {return moduleId_;}
  void mergeAndResetMEsRunSummaryCache(uint32_t run,
				       uint32_t streamId,
				       uint32_t moduleId);
  void mergeAndResetMEsLumiSummaryCache(uint32_t run,
					uint32_t lumi,
					uint32_t streamId,
					uint32_t moduleId);

 private:
  DQMStore(void) {
    if (!ibooker_)
      ibooker_ = new DQMStore::IBooker(this);
  }
  DQMStore(const DQMStore&);
  MonitorElement * book1d(std::string);
  const std::string * pwd() const;
  const std::string * me_name(std::string name);

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
};  // DQMStore

#endif

/**
 Local Variables:
 show-trailing-whitespace: t
 truncate-lines: t
 End:
*/
