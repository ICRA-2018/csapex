/// HEADER
#include "voxel_grid.h"

/// PROJECT
#include <csapex/model/box.h>
#include <csapex/model/connector_in.h>
#include <csapex/model/connector_out.h>
#include <csapex_point_cloud/point_cloud_message.h>
#include <csapex/utility/qt_helper.hpp>

/// SYSTEM
#include <pluginlib/class_list_macros.h>
#include <boost/mpl/for_each.hpp>
#include <pcl_ros/transforms.h>
#include <pcl/filters/voxel_grid.h>

PLUGINLIB_EXPORT_CLASS(csapex::VoxelGrid, csapex::BoxedObject)

using namespace csapex;
using namespace csapex::connection_types;

VoxelGrid::VoxelGrid()
{
    addTag(Tag::get("PointCloud"));
}

void VoxelGrid::fill(QBoxLayout *layout)
{
    box_->setSynchronizedInputs(true);

    input_cloud_ = new ConnectorIn(box_, 0);
    input_cloud_->setLabel("PointCloud");
    input_cloud_->setType(PointCloudMessage::make());

    output_ = new ConnectorOut(box_, 0);
    output_->setLabel("PointCloud");
    output_->setType(PointCloudMessage::make());

    box_->addInput(input_cloud_);
    box_->addOutput(output_);

    res_ = QtHelper::makeDoubleSlider(layout, "resolution", 0.1, 0.01, 1.0, 0.01);
    QObject::connect(res_, SIGNAL(valueChanged(double)), this, SLOT(update()));

    update();
}

void VoxelGrid::update()
{
    state.resolution_ = res_->doubleValue();
}

void VoxelGrid::allConnectorsArrived()
{
    PointCloudMessage::Ptr msg(input_cloud_->getMessage<PointCloudMessage>());

    boost::apply_visitor (PointCloudMessage::Dispatch<VoxelGrid>(this), msg->value);
}

template <class PointT>
void VoxelGrid::inputCloud(typename pcl::PointCloud<PointT>::Ptr cloud)
{
    Eigen::Vector4f leaf(state.resolution_, state.resolution_, state.resolution_, 0);

    typename pcl::PointCloud<PointT>::Ptr out(new pcl::PointCloud<PointT>);

    pcl::VoxelGrid<PointT> voxel_f;
    voxel_f.setInputCloud(cloud);
    voxel_f.setLeafSize(leaf);
    voxel_f.filter(*out);

    PointCloudMessage::Ptr msg(new PointCloudMessage);
    msg->value = out;

    output_->publish(msg);
}

Memento::Ptr VoxelGrid::getState() const
{
    return boost::shared_ptr<State>(new State(state));
}

void VoxelGrid::setState(Memento::Ptr memento)
{
    boost::shared_ptr<VoxelGrid::State> m = boost::dynamic_pointer_cast<VoxelGrid::State> (memento);
    assert(m.get());

    state = *m;

    res_->setDoubleValue(state.resolution_);
}

void VoxelGrid::State::writeYaml(YAML::Emitter& out) const {
    out << YAML::Key << "resolution" << YAML::Value << resolution_;
}
void VoxelGrid::State::readYaml(const YAML::Node& node) {
    if(node.FindValue("resolution")) {
        node["resolution"] >> resolution_;
    }
}
