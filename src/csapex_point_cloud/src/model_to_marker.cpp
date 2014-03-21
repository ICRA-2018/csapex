/// HEADER
#include "model_to_marker.h"

/// PROJECT
#include <csapex/model/connector_out.h>
#include <csapex/model/connector_in.h>
#include <csapex_core_plugins/ros_message_conversion.h>
//#include <csapex/model/message.h>
#include <utils_param/parameter_factory.h>



/// SYSTEM
#include <csapex/utility/register_apex_plugin.h>
#include <visualization_msgs/Marker.h>
#include <geometry_msgs/Point.h>



CSAPEX_REGISTER_CLASS(csapex::ModelToMarker, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;

ModelToMarker::ModelToMarker()
{
    //addTag(Tag::get("PointCloud"));
}

void ModelToMarker::process()
{
    boost::shared_ptr<GenericMessage<ModelMessage> > message = input_->getMessage<GenericMessage<ModelMessage> >();
    if(param<bool>("publish marker")) {
        publishMarkers(*(message->value));
    }

}

void ModelToMarker::setup()
{
    setSynchronizedInputs(true);
    input_ = addInput<GenericMessage<ModelMessage> >("ModelMessage");
    output_ = addOutput<visualization_msgs::Marker>("Marker");

    addParameter(param::ParameterFactory::declareBool("publish marker", true));
}


void ModelToMarker::publishMarkers(const ModelMessage model_message)
{
    //visualization_msgs::MarkerArray::Ptr marker_array(new visualization_msgs::MarkerArray);
    //visualization_msgs::Marker           marker;
    visualization_msgs::Marker::Ptr      marker(new visualization_msgs::Marker);

    marker->header.frame_id     = model_message.frame_id;
    marker->header.stamp        = ros::Time::now();
    marker->ns                  = "model";
    marker->id                  = 1;
    marker->action              = visualization_msgs::Marker::ADD;

    if (model_message.model_type == pcl::SACMODEL_SPHERE ) {
        marker->type                = visualization_msgs::Marker::SPHERE;

        marker->pose.position.x = model_message.coefficients->values.at(0);
        marker->pose.position.y = model_message.coefficients->values.at(1);
        marker->pose.position.z = model_message.coefficients->values.at(2);

        marker->pose.orientation.x  = 0.0;
        marker->pose.orientation.y  = 0.0;
        marker->pose.orientation.z  = 0.0;
        marker->pose.orientation.w  = 1.0;

        double min_scale = 0.001;
        double scale = model_message.coefficients->values.at(3) > min_scale ? model_message.coefficients->values.at(3) : min_scale;
        marker->scale.x = scale;
        marker->scale.y = scale;
        marker->scale.z = scale;

        marker->color.a = 0.8;
        marker->color.r = 0.0;
        marker->color.g = 1.0;
        marker->color.b = 0.0;

    } else if (model_message.model_type == pcl::SACMODEL_CONE ) {
        marker->type                = visualization_msgs::Marker::ARROW;

        geometry_msgs::Point apex;
        apex.x = model_message.coefficients->values.at(0);
        apex.y = model_message.coefficients->values.at(1);
        apex.z = model_message.coefficients->values.at(2);

        geometry_msgs::Point base;
        base.x = apex.x + model_message.coefficients->values.at(3);
        base.y = apex.y + model_message.coefficients->values.at(4);
        base.z = apex.z + model_message.coefficients->values.at(5);

        marker->points.push_back(base);
        marker->points.push_back(apex);
//        marker->pose.orientation.x  = 0.0;
//        marker->pose.orientation.y  = 0.0;
//        marker->pose.orientation.z  = 0.0;
//        marker->pose.orientation.w  = 1.0;

        double scale = 1.0;
        marker->scale.x = scale;
        marker->scale.y = scale;
        marker->scale.z = scale;

        marker->color.a = 0.8;
        marker->color.r = 1.0;
        marker->color.g = 1.0;
        marker->color.b = 0.0;

    } else {
        printf("unknown Model!!");
    }

    // Todo:
    // Add plane and cylinder
    // Add circle
    // Change to marker array so that a nicer cone can be displayed by a vector and a circle

   output_->publish<visualization_msgs::Marker>(marker);

}