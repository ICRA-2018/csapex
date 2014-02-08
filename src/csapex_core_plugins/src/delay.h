#ifndef DELAY_H
#define DELAY_H

/// PROJECT
#include <csapex/model/boxed_object.h>
#include <csapex/model/connection_type.h>

/// SYSTEM
#include <QMutex>

namespace csapex {

class Delay : public Node
{
public:
    Delay();

    virtual void allConnectorsArrived();
    virtual void setup();

private:
    ConnectorIn* input_;
    ConnectorOut* output_;
};

}

#endif // DELAY_H