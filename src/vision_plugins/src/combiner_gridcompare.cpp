#include "combiner_gridcompare.h"

/// PROJECT
#include <vision_evaluator/qt_helper.hpp>

using namespace vision_evaluator;

GridCompare::GridCompare(State::Ptr state) :
    state_(state)
{
}

void GridCompare::fill(QBoxLayout *layout)
{
    ImageCombiner::fill(layout);
    addSliders(layout);
}


void GridCompare::addSliders(QBoxLayout *layout)
{
    slide_width_  = QtHelper::makeSlider(layout, "Grid Width",  64, 1, 640);
    slide_height_ = QtHelper::makeSlider(layout, "Grid Height", 48, 1, 640);

}

void GridCompare::updateDynamicGui(QBoxLayout *layout)
{
}

/// MEMENTO
GridCompare::State::State()
{
    channel_count = 0;
    grid_height   = 64;
    grid_width    = 48;
}

void GridCompare::State::readYaml(const YAML::Node &node)
{
    node["channel_count"] >> channel_count;
    node["grid_width"] >> grid_width;
    node["grid_height"] >> grid_height;
}

void GridCompare::State::writeYaml(YAML::Emitter &out) const
{
    out << YAML::Key << "channel_count" << YAML::Value << channel_count;
    out << YAML::Key << "grid_width" << YAML::Value << grid_width;
    out << YAML::Key << "grid_height" << YAML::Value << grid_height;
}