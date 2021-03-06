/// HEADER
#include <csapex/signal/event.h>

/// COMPONENT
#include <csapex/signal/slot.h>
#include <csapex/profiling/timer.h>
#include <csapex/utility/assert.h>
#include <csapex/msg/token_traits.h>
#include <csapex/msg/no_message.h>
#include <csapex/msg/any_message.h>
#include <csapex/model/token.h>

/// SYSTEM
#include <iostream>

using namespace csapex;

Event::Event(const UUID& uuid, ConnectableOwnerWeakPtr owner) : StaticOutput(uuid, owner)
{
    setType(makeEmpty<connection_types::AnyMessage>());
}

Event::~Event()
{
}

void Event::reset()
{
    setSequenceNumber(0);
}

void Event::trigger()
{
    TokenPtr token = connection_types::makeEmptyToken<connection_types::AnyMessage>();
    triggerWith(token);
}

void Event::triggerWith(TokenPtr token)
{
    addMessage(token);
    ++count_;

    triggered();
}

bool Event::isSynchronous() const
{
    return false;
}
