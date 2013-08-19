#ifndef COMMAND_ADD_BOX_HPP
#define COMMAND_ADD_BOX_HPP

/// COMPONENT
#include <csapex/command.h>
#include <csapex/memento.h>
#include <csapex/selector_proxy.h>

/// SYSTEM
#include <QWidget>

namespace csapex
{
class Box;

namespace command
{

struct AddBox : public Command
{
    AddBox(SelectorProxy::Ptr selector_, QPoint pos_, Memento::Ptr state = Memento::NullPtr, const std::string& parent_uuid_ = "", const std::string& uuid_ = "");

protected:
    bool execute();
    bool undo();
    bool redo();

    void refresh();

private:
    SelectorProxy::Ptr selector_;
    QPoint pos_;

    std::string parent_uuid_;
    std::string uuid_;

    csapex::Box* box_;

    Memento::Ptr saved_state_;
};
}
}

#endif // COMMAND_ADD_BOX_HPP
