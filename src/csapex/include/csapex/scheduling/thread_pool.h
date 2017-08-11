#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/// COMPONENT
#include <csapex/scheduling/executor.h>
#include <csapex/scheduling/scheduling_fwd.h>
#include <csapex/core/exception_handler.h>

/// SYSTEM
#include <map>
#include <vector>
#include <set>
#include <yaml-cpp/yaml.h>
#include <csapex/utility/slim_signal.hpp>

namespace csapex
{

class CsApexCore;

class CSAPEX_EXPORT ThreadPool : public Executor
{
public:
    ThreadPool(csapex::ExceptionHandler &handler, bool enable_threading, bool grouping);
    ThreadPool(Executor* parent, csapex::ExceptionHandler &handler, bool enable_threading, bool grouping);
    ~ThreadPool();

    bool isThreadingEnabled() const;
    bool isGroupingEnabled() const;

    virtual void performStep() override;

    virtual void start() override;
    virtual void stop() override;
    virtual void clear() override;

    virtual bool isRunning() const override;

    std::size_t getGroupCount() const;
    ThreadGroup* getGroupAt(std::size_t pos);

    std::vector<ThreadGroupPtr> getGroups();
    ThreadGroup* getDefaultGroup();
    ThreadGroup* getGroup(int id);
    ThreadGroup* getGroupFor(TaskGenerator* generator);

    std::string nextName();

    virtual void add(TaskGenerator*) override;
    virtual void remove(TaskGenerator *) override;

    void usePrivateThreadFor(TaskGenerator* task);
    void addToGroup(TaskGenerator* task, int group_id);

    ThreadGroup *createGroup(const std::string &name, int id = -1);
    int createNewGroupFor(TaskGenerator *task, const std::string &name);
    void removeGroup(int id);

    void useDefaultThreadFor(TaskGenerator *task);

    void saveSettings(YAML::Node&);
    void loadSettings(YAML::Node&);

public:
    slim_signal::Signal<void (ThreadGroupPtr)> group_created;
    slim_signal::Signal<void (ThreadGroupPtr)> group_removed;

protected:
    void pauseChanged(bool pause) override;
    void steppingChanged(bool performStep) override;

    bool isStepDone() override;

    void checkIfStepCanBeDone();

private:
    void assignGeneratorToGroup(TaskGenerator* task, ThreadGroup* group);

    bool isInPrivateThread(TaskGenerator* task) const;
    bool isInGroup(TaskGenerator* task, int id) const;

//    void clearGroup(ThreadGroup *group);
    void removeGroup(ThreadGroup *group);

private:
    ExceptionHandler& handler_;

    TimedQueuePtr timed_queue_;

    bool enable_threading_;
    bool grouping_;

    ThreadGroupPtr default_group_;

    std::vector<ThreadGroupPtr> groups_;
    std::map<TaskGenerator*, ThreadGroup*> group_assignment_;
    std::map<TaskGenerator*, slim_signal::ScopedConnection> group_connection_;
};

}

#endif // THREAD_POOL_H
