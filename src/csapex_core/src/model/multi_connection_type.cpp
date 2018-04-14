/// HEADER
#include <csapex/model/multi_connection_type.h>

/// SYSTEM
#include <sstream>

using namespace csapex;

namespace {
std::string toString(const std::vector<TokenData::Ptr>& types) {
    std::stringstream ss;
    int i = 0;
    for(std::vector<TokenData::Ptr>::const_iterator it = types.begin(); it != types.end(); ++it) {
        if(i++ > 0) ss << ", ";
        ss << (*it)->typeName();
    }
    return ss.str();
}
}

MultiTokenData::MultiTokenData()
{

}

MultiTokenData::MultiTokenData(const std::vector<TokenData::Ptr>& types)
    : TokenData("one of {" + toString(types) + "}"), types_(types)
{

}

bool MultiTokenData::canConnectTo(const TokenData *other_side) const
{
    for(std::vector<TokenData::Ptr>::const_iterator it = types_.begin(); it != types_.end(); ++it) {
        if((*it)->canConnectTo(other_side)) {
            return true;
        }
    }

    return false;
}

bool MultiTokenData::acceptsConnectionFrom(const TokenData *other_side) const
{
    for(std::vector<TokenData::Ptr>::const_iterator it = types_.begin(); it != types_.end(); ++it) {
        if((*it)->acceptsConnectionFrom(other_side)) {
            return true;
        }
    }

    return false;
}

TokenData::Ptr MultiTokenData::clone() const
{
    Ptr new_msg(new MultiTokenData(types_));
    return new_msg;
}

TokenData::Ptr MultiTokenData::toType() const
{
    Ptr new_msg(new MultiTokenData(types_));
    return new_msg;
}

std::shared_ptr<Clonable> MultiTokenData::makeEmptyClone() const
{
    return std::shared_ptr<Clonable>(new MultiTokenData);
}
void MultiTokenData::serialize(SerializationBuffer &data) const
{
    throw std::runtime_error("MultiTokenData is a meta type and cannot be serialized");
}
void MultiTokenData::deserialize(const SerializationBuffer& data)
{
    throw std::runtime_error("MultiTokenData is a meta type and cannot be serialized");
}
