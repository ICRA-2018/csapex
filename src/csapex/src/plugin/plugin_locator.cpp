/// HEADER
#include <csapex/plugin/plugin_locator.h>

/// PROJECT
#include <csapex/model/node.h>
#include <utils_param/string_list_parameter.h>
#include <utils_param/parameter_factory.h>

using namespace csapex;

PluginLocator::PluginLocator(Settings &settings)
    : settings_(settings)
{
    if(settings_.knows("ignored_libraries")) {
        ignored_persistent_ = boost::dynamic_pointer_cast<param::StringListParameter>(settings_.get("ignored_libraries"));

    } else {
        std::vector<std::string> ignored_libraries(ignored_libraries_.begin(), ignored_libraries_.end());

        ignored_persistent_.reset(new param::StringListParameter("ignored_libraries", param::ParameterDescription("ignored libraries")));
        settings_.add(ignored_persistent_);
    }
    std::vector<std::string> tmp = ignored_persistent_->getValues();
    ignored_libraries_.insert(tmp.begin(), tmp.end());
}

void PluginLocator::ignoreLibrary(const std::string &name, bool ignore)
{
    if(ignore) {
        ignored_libraries_.insert(name);
        ignored_persistent_->add(name);
    } else {
        ignored_libraries_.erase(name);
        ignored_persistent_->removeAll(name);
    }
}

bool PluginLocator::isLibraryIgnored(const std::string &name) const
{
    return ignored_libraries_.find(name) != ignored_libraries_.end();
}

void PluginLocator::setLibraryLoaded(const std::string &name)
{
    loaded_libraries_.insert(name);
}

bool PluginLocator::isLibraryLoaded(const std::string &name) const
{
    return loaded_libraries_.find(name) != loaded_libraries_.end();
}


void PluginLocator::setLibraryError(const std::string& name, const std::string& error)
{
    error_libraries_[name] = error;
}

bool PluginLocator::hasLibraryError(const std::string &name) const
{
    return error_libraries_.find(name) != error_libraries_.end();
}

std::string PluginLocator::getLibraryError(const std::string &name) const
{
    return error_libraries_.at(name);
}

std::vector<std::string> PluginLocator::getAllLibraries() const
{
    std::vector<std::string> result(loaded_libraries_.begin(), loaded_libraries_.end());
    for(std::map<std::string, std::string>::const_iterator it = error_libraries_.begin();
        it != error_libraries_.end(); ++it) {
        result.push_back(it->first);
    }

    result.insert(result.end(), ignored_libraries_.begin(), ignored_libraries_.end());

    std::sort(result.begin(), result.end());

    return result;
}
