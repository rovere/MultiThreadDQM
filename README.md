MultiThreadDQM
==============

This is a simple, stand-alone, toy model to test the feasibility of
having a multithreaded DQM Framework inside CMSSW.

The main driving ideas, so far, are exposed in the next sections.

Prerequisites
-------------

The example is thuoght to be run in stand-alone mode, so you do not
need to have a full blown CMSSW area avaiable. This is also extremely
handy since you can even test the code on your private
laptop/desktop. There are , though, few requirements:

* you need to use a relatively recent compiler. So far I only tested
  the code against g++ 4.7 and 4.8.
* you need to have [ROOT](http://root.cern.ch) installed and available
  in your env.

Installation
--------------------

The compilation command is located at the first line of the source
code.

Keep the DQMStore as the unique, central storage of all MonitorElements
-----------------------------------------------------------------------

The current design of the DQM Framework covers all the use cases that
we have to take care of, from Online processing and network shipping
of MonitorElements from single Online Applications to the central DQM
GUI, Offline processing, Monte Carlo, you name them. I would like to
change as few things as possible and try to reuse as much of the core
code as possible, since it proved to be solid rock over the past 3+
years of data taking.

The main idea I have to solve the multithread issue could be
summarized into **multiply and serialize** motto. **Multiply** since
all DQM Modules (except for the ones run in the harvestin step, which
will be execute serially, no gain there!) will become Stream
Modules(SM), _each with its own local instance of
MonitorElements(ME)_, all of them anyway tracked into the `DQMStore`
internal storage: this will avoid problems on concurrent operations on
the same ME.  **Serialise** since all DQM Modules will be extended
using `RunSummaryCache<void>` and `LuminositySummaryCache<void>`, to
have hooks to methods that are guaranteed by the framework to be
executed sequentially. The only caveat on this approach is the booking
time (at the beginRun), since at that point in time I have no
guarantee of sequential execution. I have ideas on how to solve this,
but the bottom line is that the booking must be a protected(locked)
operation on the `DQMStore`.

The `DQMStore` will keep:

* `std::set<MonitorElement> data_;`
* `std::set<std::string> dirs_;`

The internal `std::string pwd_` has been kept as long as it is
completely hidden to the outside (i.e. it will make no sense to ask
the `DQMStore` in which directory we are, but it will make sense for
the `DQMStore` to be in one specific directory for a specific period
of consecutive (lock guarded) time. The same comment applies for the
`run_`, `streamId_` and `moduleId_` data members.

In addition to this I added a `std::set<std::string> objs_` in order
to avoid unnecessary duplication of histogram names, at least in the
`DQMStore` itself: **the ones into ROOT are unavoidable**.

Sets are extremely handy in this situation since they are "immutable"
in the sense that pointers to their elements will remain valid across
insertions (of course provided the element is not removed/deleted).

MonitorElement's additions
--------------------------

In oder to be able to handle and disantengle all the different MEs
into the `DQMStore` correctly, the ME has been extended to include:

   * Run Number information (uint_32, since I do not want to depend on
     additional CMSSW classes that I have to import into the DQMGUI).

   * Stream ID (uint_32, same as before)

   * Module Unique ID (uint_32, same as before)

For the last two points above we have to agree on some "dummy value"
that will be used by the `DQMStore` internally to distinguish the real
**Summary ME** (*that will integrate statistics across
Streams/Modules*) and the local copies (Stream and Module ID
dependent). The current ordering of the set is:

* Run Ordered
* Stream Ordered
* Folder+Name ordered (the current ordering of the set<ME>)

I do not include the Module ID in the ordering function since I will
enforce the fact that no 2 elements with the same Run, Stream,
Folder+Name will exist: if I do not do that, the guarantee of having
unique local objects for each stream will not be valid any longer. On
the other hand there is clear access pattern based also on the
ModuleID info, so it could be handy to have them sorted also by this
criteria.

Global Merging Logic
--------------------

At specific point in time the merging of local information into a
global one has to be triggered. The current proposal is to trigger
that during end{Run,LS}Summary events.

When, e.g., an `endLuminositySummary` for module A in stream 1 is
triggered, the module will tell the `DQMStore` to take the histograms
he booked for the current Run (via its StreamID+moduleID) and to merge
them into the global ME that will have to integrate all info for the
same Run from different streams. I have the guarantee from the
framework that the "same" (i.e., different instance, since it's a
stream module) module A on stream 2 will eventually trigger an
`endRunSummary` event **at a different time, either before or after
but never concurrently**: this guarantees that the access to the
global ME is serial, avoiding all multithread related problems.

Also, concurrent `endRunSummary` events of different modules(C++ type,
not instance) for the same or different streams/Runs does not
represent a problem, since they will always operate on different
memory locations, i.e. different MEs, avoiding all multi-thread
related headaches.

Booking Logic
-------------

The booking process is the weak point, since I have to protect and
serialize access to the internal storage of the `DQMStore` (the
sets). The implemented solution is to treat booking operations inside
every single module as a **transaction**: the module will tell the
`DQMStore` he wants to start booking. The `DQMStore` will return an
object (`IBooker`) holding a central (i.e. owned by the `DQMStore`)
lock and the module will use this object to book histograms, the facto
holding a lock for as long as needed to book all histograms. In this
picture it will help to have a `pwd_` state of `DQMStore` that for the
duration of this transaction will have a correct meaning, avoiding the
unnecessary pain of passing the directory to each and every book
call. Also the Run/StreamID/ModuleID are centrally registered to this
transaction object once and propagated properly to all the MEs booked
within the transaction (thanks @fwyzard for the nice suggestion and
@Dr15Jones for the nice suggestion about the smart usage of lambda
functions for this very same purpose). This will basically leave the
current booking interface as is, provided the users will use the
transaction object and not the `DQMStore`. To enforce this we can move
the book methods of `DQMStore` as private, so noboby but the `IBooker`
will be able to see them. The **transaction** is such that the lock it
holds will be released automatically when it ends.

Online DQM
----------

The online DQM is a little bit fishy, in the sense that with the
proposal I am making, we no longer have a unique, cumulative, truly
dynamic view of what is going on. The live aspect is kept only in the
single streams, whereas the cumulative nature is only available at
specific points in time, after the end{Luminosity,Run}Summary events
have been triggered. I have 2 options here:

* Use a specific stream as the unique source of information to be sent
  via network to the DQM GUI: this will guarentee the coherence (no
  flips between "same" histograms from different streams!!), but will
  limit the statistics to a single stream.

* send only the cumulative ones: this will maximise the statistics,
  but will spoil the live nature of the monitoring: we will have
  updates only after the end{LS,Run}Summary have been triggered.

I still have not clear preference, even though I guess I do prefer the
second option, as long as we are fast enough in cutting down events
from LS if we are behing data taking.

The other major challenge is also how important information is derived
in Online DQM Applications. The typical example is the beamspot one: I
do not want to have a beamspot computed by all streams, one per
stream-module, but rather prefer to have 1 beamspot computed out of
all the statistics (in the online way, i.e. all I could process in
23s) of a LS. This imposes also some strict policy on the algorithm
itself, not only on the thread-safeness of the histograms. This points
is still to be investigated for all such modules.
