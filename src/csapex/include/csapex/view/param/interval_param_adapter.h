#ifndef INTERVAL_PARAM_ADAPTER_H
#define INTERVAL_PARAM_ADAPTER_H

/// COMPONENT
#include <csapex/view/param/param_adapter.h>
#include <csapex/param/interval_parameter.h>

class QHBoxLayout;

namespace csapex
{
class ParameterContextMenu;

class IntervalParameterAdapter : public ParameterAdapter
{
public:
    IntervalParameterAdapter(param::IntervalParameter::Ptr p);

    virtual void setup(QBoxLayout* layout, const std::string& display_name) override;

private:
    template <typename T, typename Slider, typename Spinbox>
    void genericSetup();

private:
    param::IntervalParameterPtr interval_p_;

    QHBoxLayout* internal_layout;
};


}

#endif // INTERVAL_PARAM_ADAPTER_H
