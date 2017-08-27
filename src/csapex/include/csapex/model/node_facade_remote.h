#ifndef NODE_FACADE_REMOTE_H
#define NODE_FACADE_REMOTE_H

/// PROJECT
#include <csapex/model/node_facade.h>
#include <csapex/io/io_fwd.h>
#include <csapex/serialization/serializable.h>

namespace csapex
{

class CSAPEX_EXPORT NodeFacadeRemote : public NodeFacade
{
public:
    NodeFacadeRemote(SessionPtr session,
                     NodeHandlePtr nh, NodeWorkerPtr nw, NodeRunnerPtr nr);

    ~NodeFacadeRemote();

    std::string getType() const override;
    UUID getUUID() const override;

    bool isActive() const override;
    void setActive(bool active) override;

    bool isProcessingEnabled() const override;

    bool isGraph() const override;
    AUUID getSubgraphAUUID() const override;

    bool isSource() const override;
    bool isSink() const override;
    bool isProcessingNothingMessages() const override;

    bool isParameterInput(const UUID& id) override;
    bool isParameterOutput(const UUID& id) override;

    bool isVariadic() const override;
    bool hasVariadicInputs() const override;
    bool hasVariadicOutputs() const override;
    bool hasVariadicEvents() const override;
    bool hasVariadicSlots() const override;

    std::vector<ConnectorDescription> getInputs() const override;
    std::vector<ConnectorDescription> getOutputs() const override;
    std::vector<ConnectorDescription> getEvents() const override;
    std::vector<ConnectorDescription> getSlots() const override;

    std::vector<ConnectorDescription> getInternalInputs() const override;
    std::vector<ConnectorDescription> getInternalOutputs() const override;
    std::vector<ConnectorDescription> getInternalEvents() const override;
    std::vector<ConnectorDescription> getInternalSlots() const override;

    NodeCharacteristics getNodeCharacteristics() const override;

    bool isProfiling() const override;
    void setProfiling(bool profiling) override;

    bool isError() const override;
    ErrorState::ErrorLevel errorLevel() const override;
    std::string errorMessage() const override;

    ExecutionState getExecutionState() const override;

    std::string getLabel() const override;

    double getExecutionFrequency() const override;
    double getMaximumFrequency() const override;

    // Parameterizable
    virtual std::vector<param::ParameterPtr> getParameters() const override;
    param::ParameterPtr getParameter(const std::string& name) const override;
    bool hasParameter(const std::string& name) const override;

//    template <typename T>
//    T readParameter(const std::string& name) const;
//    template <typename T>
//    void setParameter(const std::string& name, const T& value);

    // Debug Access
    std::string getDebugDescription() const override;
    std::string getLoggerOutput(ErrorState::ErrorLevel level) const override;

    // TODO: proxies
    ProfilerPtr getProfiler() override;

    NodeStatePtr getNodeState() const override;
    NodeStatePtr getNodeStateCopy() const override;
    void setNodeState(NodeStatePtr memento) override;

    GenericStateConstPtr getParameterState() const override;

    NodeHandlePtr getNodeHandle() const;
    NodeWorkerPtr getNodeWorker() const;
    NodeRunnerPtr getNodeRunner() const;
    NodePtr getNode() const;

public:
    slim_signal::ObservableSignal<void(SerializableConstPtr)> remote_data_connection;

private:
    void connectNodeHandle();
    void connectNodeWorker();

private:
    SessionPtr session_;

    NodeHandlePtr nh_;
    NodeWorkerPtr nw_;
    NodeRunnerPtr nr_;
};

}

#endif // NODE_FACADE_REMOTE_H
