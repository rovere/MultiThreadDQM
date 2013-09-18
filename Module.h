#ifndef Module_H
#define Module_H

#include <string>
#include <vector>

class MonitorElement;

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

#endif

/**
   Local Variables:
   show-trailing-whitespace: t
   truncate-lines: t
   End:
*/
