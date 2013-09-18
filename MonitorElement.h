#ifndef MonitorElement_H
#define MonitorElement_H

#include <TH1F.h>

#include <cassert>
#include <string>

class DQMStore;

class MonitorElement {
 public:
  friend class DQMStore;
 MonitorElement(const std::string *dir,
                const std::string *name,
                uint32_t run,
                uint32_t streamId,
                uint32_t moduleId)
   :run_(run),
    lumi_(0),
    streamId_(streamId),
    moduleId_(moduleId),
    lumiFlag_(0),
    histo_(nullptr),
    dirname_(dir),
    name_(name) {
    }

  MonitorElement(const MonitorElement &rhs);

  // for ordering into set.
  bool operator< (MonitorElement const & rhs) const;
  uint32_t run() const {return run_;}
  uint32_t lumi() const {return lumi_;}
  uint32_t streamId() const {return streamId_;}
  uint32_t moduleId() const {return moduleId_;}
  std::string dir() const {return *dirname_;}
  std::string name() const {return *name_;}
  double mean() const {return histo_->GetMean();}
  double rms() const {return histo_->GetRMS();}
  double entries() const {return histo_->GetEntries();}
  TH1 * getTH1() const {return histo_;}
  void setLumi(uint32_t ls) {lumi_ = ls;}
  void setLumiFlag() {lumiFlag_ = 1;}
  bool getLumiFlag() const {return lumiFlag_;}
  MonitorElement * initialize(TH1 * obj) {
    if (obj)
      histo_ = obj;
    return this;
  }

  void Fill(double val) {
    if (histo_)
      histo_->Fill(val);
  }
  void reset();

 private:
  uint32_t run_;
  uint32_t lumi_;
  uint32_t streamId_;
  uint32_t moduleId_;
  uint32_t lumiFlag_;
  TH1 * histo_;
  const std::string * dirname_;
  const std::string * name_;

  void globalize() {
    streamId_ = 0;
    moduleId_ = 0;
  }
};  // MonitorElement


#endif

/**
   Local Variables:
   show-trailing-whitespace: t
   truncate-lines: t
   End:
*/
