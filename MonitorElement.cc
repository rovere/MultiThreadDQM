#include <MonitorElement.h>

MonitorElement::MonitorElement(const MonitorElement & rhs) {
  run_ = rhs.run_;
  lumi_ = rhs.lumi_;
  streamId_ = rhs.streamId_;
  moduleId_ = rhs.moduleId_;
  lumiFlag_ = rhs.lumiFlag_;
  histo_ = static_cast<TH1 *> (rhs.histo_->Clone());
  dirname_ = rhs.dirname_;
  name_ = rhs.name_;
}

bool MonitorElement::operator< (const MonitorElement & rhs) const {
  if (run_ == rhs.run_) {
    if (lumi_ == rhs.lumi_) {
      if (streamId_ == rhs.streamId_) {
	if (moduleId_ == rhs.moduleId_) {
	  if (*dirname_ == *rhs.dirname_) {
	    return *name_ < *rhs.name_;
	  }
	  return *dirname_ < *rhs.dirname_;
	}
	return moduleId_ < rhs.moduleId_;
      }
      return streamId_ < rhs.streamId_;
    }
    return lumi_ < rhs.lumi_;
  }
  return run_ < rhs.run_;
  assert(false);
}

void MonitorElement::reset() {
  histo_->Reset("ICES");
}
/**
   Local Variables:
   show-trailing-whitespace: t
   truncate-lines: t
   End:
*/
