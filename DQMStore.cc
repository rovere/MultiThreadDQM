#include <DQMStore.h>
#include <MonitorElement.h>

#include <utility>
#include <iostream>
#include <iomanip>

void dumpMe(const MonitorElement & me,
            bool printStat = false) {
  std::cout << "Run: " << me.run()
	    << " Lumi: " << me.lumi()
            << " LumiFlag: " << me.getLumiFlag()
            << " streamId: " << me.streamId()
            << " moduleId: " << me.moduleId()
            << " fullpathname: " << me.dir()
            << "/" << me.name();
  if (printStat)
    std::cout << " Mean: " << me.mean()
              << " RMS: " << me.rms()
              << " Entries: "
              << std::setprecision(9) << me.entries();
  std::cout << std::endl;
}

void DQMStore::dump(bool printStat) {
  unsigned int n = 0;
  for (auto item : data_) {
    std::cout << ++n << " --> ";
    dumpMe(item, printStat);
  }
}

void DQMStore::cd(std::string dir) {
  if (!dirs_.count(dir))
    dirs_.insert(dir);
  pwd_ = dir;
}

const std::string * DQMStore::pwd() const  {
  assert(dirs_.count(pwd_));
  return &(*dirs_.find(pwd_));
}

const std::string * DQMStore::me_name(std::string name) {
  if (!objs_.count(name))
    return &(*objs_.insert(name).first);
  return &(*objs_.find(name));
}


MonitorElement * DQMStore::book1d(std::string name) {
  MonitorElement me(pwd(), me_name(name), run_, streamId_, moduleId_);
  TH1F * tmp = new TH1F(name.c_str(), name.c_str(), 1000, 0., 1000.);
  me.initialize(tmp);
  return &(const_cast<MonitorElement &>(*(data_.insert(me).first)));
}

/** Function to transfer the local copies of histograms from each
    stream into the global, final ROOT Object. Since this involves a
    de-facto booking action in the case in which the global object is
    not yet there, the function requires the acquisition of the
    central lock into the DQMStore. A double find is done on the
    internal data_ data since we have no guarantee that a previous
    module holding the lock that is stopping us have booked what we
    looked for before requiring the lock. In case we book the global
    object for the first time, no Add action is needed since the ROOT
    histograms is cloned starting from the local one. */

void DQMStore::mergeAndResetMEsRunSummaryCache(uint32_t run,
					       uint32_t streamId,
					       uint32_t moduleId) {
  std::cout << "Merging objects from run: "
            << run
	    << ", stream: " << streamId
            << " module: " << moduleId << std::endl;
  std::string null_str("");
  MonitorElement proto(&null_str, &null_str, run, streamId, moduleId);
  std::set<MonitorElement>::const_iterator e = data_.end();
  std::set<MonitorElement>::const_iterator i = data_.lower_bound(proto);
  while (i != e) {
    if (i->run() != run
        || i->streamId() != streamId
        || i->moduleId() != moduleId)
      break;
    MonitorElement global_me(*i);

    // Handle LS-based histograms in the LuminositySummaryCache() and
    // ignore them here.

    if (global_me.getLumiFlag()) {
      ++i;
      continue;
    }

    global_me.globalize();
    std::set<MonitorElement>::const_iterator me = data_.find(global_me);
    if (me != data_.end()) {
      std::cout << "Found global Object, using it. ";
      me->getTH1()->Add(i->getTH1());
      dumpMe(*me, true);
    } else {
      // Since this is equivalent to a real booking operation it must
      // be locked.
      std::cout << "No global Object found. ";
      std::lock_guard<std::mutex> guard(book_mutex_);
      me = data_.find(global_me);
      if (me != data_.end()) {
        me->getTH1()->Add(i->getTH1());
        dumpMe(*me, true);
      } else {
        std::pair<std::set<MonitorElement>::const_iterator, bool> gme;
        gme = data_.insert(global_me);
        assert(gme.second);
        dumpMe(*(gme.first), true);
      }
    }
    // TODO(rovere): eventually reset the local object and mark it as reusable??
    ++i;
  }
}

void DQMStore::mergeAndResetMEsLumiSummaryCache(uint32_t run,
						uint32_t lumi,
						uint32_t streamId,
						uint32_t moduleId) {
  std::cout << "Merging objects from run: "
            << run << " lumi: " << lumi
	    << ", stream: " << streamId
            << " module: " << moduleId << std::endl;
  std::string null_str("");
  MonitorElement proto(&null_str, &null_str, run, streamId, moduleId);
  std::set<MonitorElement>::const_iterator e = data_.end();
  std::set<MonitorElement>::const_iterator i = data_.lower_bound(proto);
  while (i != e) {
    if (i->run() != run
        || i->streamId() != streamId
        || i->moduleId() != moduleId)
      break;
    MonitorElement global_me(*i);

    // Handle LS-based histograms in the LuminositySummaryCache() and
    // ignore them here.

    if (!global_me.getLumiFlag()) {
      ++i;
      continue;
    }

    global_me.globalize();
    global_me.setLumi(lumi);
    std::set<MonitorElement>::const_iterator me = data_.find(global_me);
    if (me != data_.end()) {
      std::cout << "Found global Object, using it --> ";
      me->getTH1()->Add(i->getTH1());
      dumpMe(*me, true);
    } else {
      // Since this is equivalent to a real booking operation it must
      // be locked.
      std::cout << "No global Object found. ";
      std::lock_guard<std::mutex> guard(book_mutex_);
      me = data_.find(global_me);
      if (me != data_.end()) {
        me->getTH1()->Add(i->getTH1());
        dumpMe(*me, true);
      } else {
        std::pair<std::set<MonitorElement>::const_iterator, bool> gme;
        gme = data_.insert(global_me);
        assert(gme.second);
        dumpMe(*(gme.first), true);
      }
    }
    const_cast<MonitorElement*>(&*i)->reset();
    dumpMe(*i, true);
    // TODO(rovere): eventually reset the local object and mark it as reusable??
    ++i;
  }
}

void DQMStore::IBooker::cd(std::string dir) {
  dqmstore_->cd(dir);
}

MonitorElement * DQMStore::IBooker::book1d(std::string name) {
  return dqmstore_->book1d(name);
}

/**
 Local Variables:
 show-trailing-whitespace: t
 truncate-lines: t
 End:
*/
