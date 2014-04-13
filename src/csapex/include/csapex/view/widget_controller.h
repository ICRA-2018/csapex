#ifndef WIDGET_CONTROLLER_H
#define WIDGET_CONTROLLER_H

/// PROJECT
#include <csapex/csapex_fwd.h>
#include <csapex/utility/uuid.h>
#include <csapex/model/box_selection_model.h>
#include <csapex/model/connection_selection_model.h>

/// SYSTEM
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QPoint>
#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

namespace csapex
{

class WidgetController : public QObject
{
    Q_OBJECT

public:
    typedef boost::shared_ptr<WidgetController> Ptr;

public:
    WidgetController(GraphPtr graph);

    Box* getBox(const UUID& node_id);

    Port* getPort(const UUID& connector_id);
    Port* getPort(const Connectable* connector_id);

    GraphPtr getGraph();

    void setDesigner(Designer* designer);
    void setCommandDispatcher(CommandDispatcher *dispatcher);

    void foreachBox(boost::function<void (Box*)> f, boost::function<bool (Box*)> pred);

public Q_SLOTS:
    void nodeAdded(NodePtr node);
    void nodeRemoved(NodePtr node);

    void connectorAdded(Connectable *connector);
    void connectorRemoved(Connectable *connector);

private:
    GraphPtr graph_;
    CommandDispatcher* dispatcher_;

public:
    BoxSelectionModel box_selection_;
    ConnectionSelectionModel connection_selection_;

private:
    Designer* designer_;
    boost::unordered_map<UUID, Box*, UUID::Hasher> box_map_;
    boost::unordered_map<UUID, Port*, UUID::Hasher> port_map_;
};

}

#endif // WIDGET_CONTROLLER_H
