/// HEADER
#include <csapex/command/delete_connection.h>

/// COMPONENT
#include <csapex/command/command.h>
#include <csapex/command/command_factory.h>
#include <csapex/model/node_handle.h>
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/model/graph.h>
#include <csapex/model/node.h>
#include <csapex/msg/bundled_connection.h>

using namespace csapex;
using namespace csapex::command;

DeleteConnection::DeleteConnection(Connectable* a, Connectable* b)
    : Meta("delete connection and fulcrums"), from_uuid(UUID::NONE), to_uuid(UUID::NONE)
{
    if((a->isOutput() && b->isInput())) {
        from_uuid = a->getUUID();
        to_uuid =  b->getUUID();

    } else if(a->isInput() && b->isOutput()) {
        from_uuid = b->getUUID();
        to_uuid =  a->getUUID();
    }
}

std::string DeleteConnection::getType() const
{
    return "DeleteConnection";
}

std::string DeleteConnection::getDescription() const
{
    return std::string("deleted connection between ") + from_uuid.getFullName() + " and " + to_uuid.getFullName();
}


bool DeleteConnection::doExecute()
{
    const auto& graph = getRootGraph();

    ConnectionPtr connection = graph->getConnection(from_uuid, to_uuid);

    connection_id = graph->getConnectionId(connection);

    locked = false;
    clear();
    add(CommandFactory(graph).deleteAllConnectionFulcrumsCommand(connection));
    locked = true;

    if(Meta::doExecute()) {
        graph->deleteConnection(connection);
    }

    return true;
}

bool DeleteConnection::doRedo()
{
    return doExecute();
}
