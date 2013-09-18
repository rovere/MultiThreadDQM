#ifndef Utils_H
#define Utils_h

#include <DQMStore.h>
#include <MonitorElement.h>

#include <iomanip>
#include <iostream>

template<typename A>
void join_and_dump(A& a,
		   DQMStore * store,
		   bool print_stat = false ) {
  for (unsigned int i = 0; i < a.size(); ++i)
    a[i].join();
  store->dump(print_stat);
}

#endif

/**
   Local Variables:
   show-trailing-whitespace: t
   truncate-lines: t
   End:
*/
