#ifndef SCHEDULING_FWD_H
#define SCHEDULING_FWD_H

/// shared_ptr
#include <memory>

#define FWD(name)                                                                                                                                                                                      \
    class name;                                                                                                                                                                                        \
    typedef std::shared_ptr<name> name##Ptr;                                                                                                                                                           \
    typedef std::unique_ptr<name> name##UniquePtr;                                                                                                                                                     \
    typedef std::weak_ptr<name> name##WeakPtr;                                                                                                                                                         \
    typedef std::shared_ptr<const name> name##ConstPtr;

namespace csapex
{
FWD(Executor)
FWD(Scheduler)
FWD(TaskGenerator)
FWD(ThreadPool)
FWD(ThreadGroup)
FWD(Task)
FWD(TimedQueue)
}  // namespace csapex

#undef FWD

#endif  // SCHEDULING_FWD_H
