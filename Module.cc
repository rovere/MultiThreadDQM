#include <Module.h>
#include <DQMStore.h>
#include <MonitorElement.h>

const int NUM_FILLS = 10000000;

void Module::bookHistograms(std::string dir,
                            uint32_t run,
                            uint32_t streamId,
                            uint32_t moduleId) {
  DQMStore * store = DQMStore::getInstance();
  store->bookTransition([&](DQMStore::IBooker & b) {
      b.cd(dir);
      mes_.push_back(b.book1d(std::string("pippo")));
      mes_.push_back(b.book1d(std::string("pluto")));
      if ((streamId%2) == 0) {
        mes_.push_back(b.book1d(std::string("paperino")));
	MonitorElement & me = *mes_.back();
	me.setLumiFlag();
      }
    }, run, streamId, moduleId);
};

void Module::fillHistograms(double val) {
  for (auto & histo : mes_)
    for (int j = 0; j < NUM_FILLS; ++j)
      histo->Fill(val);
};

/**
   Local Variables:
   show-trailing-whitespace: t
   truncate-lines: t
   End:
*/
