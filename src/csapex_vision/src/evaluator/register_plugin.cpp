/// HEADER
#include "register_plugin.h"

/// COMPONENT
#include <csapex_vision/messages_default.hpp>

/// PROJECT
#include <csapex/connection_type_manager.h>
#include <csapex/tag.h>

/// SYSTEM
#include <boost/bind.hpp>
#include <pluginlib/class_list_macros.h>
#include <QMetaType>

PLUGINLIB_EXPORT_CLASS(csapex::RegisterPlugin, csapex::CorePlugin)

using namespace csapex;

RegisterPlugin::RegisterPlugin()
{
}

void RegisterPlugin::init()
{
    Tag::createIfNotExists("Vision");
    Tag::createIfNotExists("Filter");
    Tag::createIfNotExists("Image Combiner");

    qRegisterMetaType<cv::Mat>("cv::Mat");

    ConnectionTypeManager::registerMessage("cv::Mat", boost::bind(&connection_types::CvMatMessage::make));

    ConnectionType::default_.reset(new connection_types::CvMatMessage);
}
