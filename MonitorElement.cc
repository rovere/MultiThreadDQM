#include <MonitorElement.h>

MonitorElement::MonitorElement(const MonitorElement &rhs) {
  run_ = rhs.run_;
  streamId_ = rhs.streamId_;
  moduleId_ = rhs.moduleId_;
  histo_ = static_cast<TH1 *> (rhs.histo_->Clone());
  dirname_ = rhs.dirname_;
  name_ = rhs.name_;
}

bool MonitorElement::operator< (MonitorElement const & rhs) const {
  if (run_ == rhs.run()) {
    if (streamId_ == rhs.streamId()) {
      if (moduleId_ == rhs.moduleId()) {
        if (*dirname_ == rhs.dir()) {
          return *name_ < rhs.name();
        }
        return *dirname_ < rhs.dir();
      }
      return moduleId_ < rhs.moduleId();
    }
    return streamId_ < rhs.streamId();
  }
  return run_ < rhs.run();
  assert(false);
}

/**
   Local Variables:
   show-trailing-whitespace: t
   truncate-lines: t
   End:
*/
