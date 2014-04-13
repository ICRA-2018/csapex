/// HEADER
#include <csapex/view/selectable.h>

using namespace csapex;

Selectable::Selectable()
    : selected_(false)
{
}

bool Selectable::isSelected() const
{
    return selected_;
}

void Selectable::setSelected(bool selected)
{
    selected_ = selected;

    if(selected_) {
        selectEvent();
    } else {
        deselectEvent();
    }

    selection(selected_);
}

void Selectable::selectEvent()
{

}

void Selectable::deselectEvent()
{

}
