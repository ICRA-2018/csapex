/// HEADER
#include <csapex/manager/box_manager.h>

/// COMPONENT
#include <csapex/command/delete_node.h>
#include <csapex/command/meta.h>
#include <csapex/model/boxed_object.h>
#include <csapex/model/graph.h>
#include <csapex/model/node_constructor.h>
#include <csapex/model/node.h>
#include <csapex/model/tag.h>
#include <csapex/utility/uuid.h>
#include <csapex/view/box.h>
#include <csapex/view/default_node_adapter.h>
#include <utils_plugin/plugin_manager.hpp>


/// SYSTEM
#include <boost/foreach.hpp>
#include <QApplication>
#include <QTreeWidget>
#include <stack>
#include <QDrag>
#include <qmime.h>
#include <QStandardItemModel>

using namespace csapex;

BoxManager::BoxManager()
    : node_manager_(new PluginManager<Node> ("csapex::Node")),
      node_adapter_manager_(new PluginManager<NodeAdapterBuilder> ("csapex::NodeAdapterBuilder")),
      dirty_(false)
{
    node_manager_->loaded.connect(loaded);
    node_adapter_manager_->loaded.connect(loaded);
}

namespace {
bool compare (NodeConstructor::Ptr a, NodeConstructor::Ptr b) {
    const std::string& as = UUID::stripNamespace(a->getType());
    const std::string& bs = UUID::stripNamespace(b->getType());
    return as.compare(bs) < 0;
}
}

void BoxManager::stop()
{
    if(node_manager_) {
        delete node_manager_;
        node_manager_ = NULL;
    }

    node_adapter_builders_.clear();
    if(node_adapter_manager_) {
        delete node_adapter_manager_;
        node_adapter_manager_ = NULL;
    }
}

void BoxManager::setStyleSheet(const QString &str)
{
    style_sheet_ = str;
}

BoxManager::~BoxManager()
{
    stop();
}

void BoxManager::reload()
{
    node_manager_->reload();
    node_adapter_manager_->reload();
    rebuildPrototypes();

    rebuildMap();
}

void BoxManager::rebuildPrototypes()
{
    available_elements_prototypes.clear();
    node_adapter_builders_.clear();

    typedef std::pair<std::string, DefaultConstructor<Node> > NODE_PAIR;
    Q_FOREACH(const NODE_PAIR& p, node_manager_->availableClasses()) {
        csapex::NodeConstructor::Ptr constructor(new csapex::NodeConstructor(
                                                     *settings_,
                                                     p.second.getType(), p.second.getDescription(),
                                                     p.second));
        register_box_type(constructor, true);
    }


    typedef std::pair<std::string, DefaultConstructor<NodeAdapterBuilder> > ADAPTER_PAIR;
    Q_FOREACH(const ADAPTER_PAIR& p, node_adapter_manager_->availableClasses()) {
        NodeAdapterBuilder::Ptr builder = p.second.construct();
        node_adapter_builders_[builder->getWrappedType()] = builder;
    }
}

void BoxManager::rebuildMap()
{
    Tag::createIfNotExists("General");
    Tag general = Tag::get("General");

    tag_map_.clear();
    tags_.clear();

    tags_.insert(general);

    for(std::vector<NodeConstructor::Ptr>::iterator
        it = available_elements_prototypes.begin();
        it != available_elements_prototypes.end();) {

        const NodeConstructor::Ptr& p = *it;

        try {
            bool has_tag = false;
            Q_FOREACH(const Tag& tag, p->getTags()) {
                tag_map_[tag].push_back(p);
                tags_.insert(tag);
                has_tag = true;
            }

            if(!has_tag) {
                tag_map_[general].push_back(p);
            }

            ++it;

        } catch(const NodeConstructor::NodeConstructionException& e) {
            std::cerr << "warning: cannot load node: " << e.what() << std::endl;
            it = available_elements_prototypes.erase(it);
        }
    }

    Q_FOREACH(const Tag& cat, tags_) {
        std::sort(tag_map_[cat].begin(), tag_map_[cat].end(), compare);
    }

    dirty_ = false;
}

void BoxManager::ensureLoaded()
{
    if(!node_manager_->pluginsLoaded()) {
        node_manager_->reload();
        node_adapter_manager_->reload();

        rebuildPrototypes();

        dirty_ = true;
    }

    if(dirty_) {
        rebuildMap();
    }
}

void BoxManager::insertAvailableNodeTypes(QMenu* menu)
{
    ensureLoaded();

    Q_FOREACH(const Tag& tag, tags_) {
        QMenu* submenu = new QMenu(tag.getName().c_str());
        menu->addMenu(submenu);

        Q_FOREACH(const NodeConstructor::Ptr& proxy, tag_map_[tag]) {
            QIcon icon = proxy->getIcon();
            QAction* action = new QAction(UUID::stripNamespace(proxy->getType()).c_str(), submenu);
            action->setData(QString(proxy->getType().c_str()));
            if(!icon.isNull()) {
                action->setIcon(icon);
                action->setIconVisibleInMenu(true);
            }
            action->setToolTip(proxy->getDescription().c_str());
            submenu->addAction(action);
        }
    }

    menu->menuAction()->setIconVisibleInMenu(true);

}

void BoxManager::insertAvailableNodeTypes(QTreeWidget* tree)
{
    ensureLoaded();

    tree->setDragEnabled(true);

    Q_FOREACH(const Tag& tag, tags_) {

        QTreeWidgetItem* submenu = new QTreeWidgetItem;
        submenu->setText(0, tag.getName().c_str());
        tree->addTopLevelItem(submenu);

        Q_FOREACH(const NodeConstructor::Ptr& proxy, tag_map_[tag]) {
            QIcon icon = proxy->getIcon();
            std::string name = UUID::stripNamespace(proxy->getType());

            QTreeWidgetItem* child = new QTreeWidgetItem;
            child->setToolTip(0, (proxy->getType() + ": " + proxy->getDescription()).c_str());
            child->setIcon(0, icon);
            child->setText(0, name.c_str());
            child->setData(0, Qt::UserRole, Box::MIME);
            child->setData(0, Qt::UserRole + 1, proxy->getType().c_str());

            submenu->addChild(child);
        }
    }
}


QAbstractItemModel* BoxManager::listAvailableBoxedObjects()
{
    ensureLoaded();

    QStandardItemModel* model = new QStandardItemModel;//(types, 1);

    Q_FOREACH(const NodeConstructor::Ptr& proxy, available_elements_prototypes) {
        QString name = QString::fromStdString(UUID::stripNamespace(proxy->getType()));
        QString descr(proxy->getDescription().c_str());
        QString type(proxy->getType().c_str());

        QStringList tags;
        Q_FOREACH(const Tag& tag, proxy->getTags()) {
            tags << tag.getName().c_str();
        }

        QStandardItem* item = new QStandardItem(proxy->getIcon(), type);
        item->setData(type, Qt::UserRole);
        item->setData(descr, Qt::UserRole + 1);
        item->setData(name, Qt::UserRole + 2);
        item->setData(tags, Qt::UserRole + 3);

        model->appendRow(item);
    }

    return model;
}
void BoxManager::register_box_type(NodeConstructor::Ptr provider, bool suppress_signals)
{
    available_elements_prototypes.push_back(provider);
    dirty_ = true;

    if(!suppress_signals) {
        new_box_type();
    }
}

namespace {
QPixmap createPixmap(const std::string& label, const NodePtr& content, const QString& stylesheet)
{
    csapex::Box::Ptr object;

    if(BoxManager::typeIsTemplate(content->getType())) {
        throw std::runtime_error("Groups are not implemented");
//        object.reset(new csapex::Group(""));
    } else {
        BoxedObjectPtr bo = boost::dynamic_pointer_cast<BoxedObject> (content);
        if(bo) {
            object.reset(new csapex::Box(bo));
        } else {
            object.reset(new csapex::Box(content, NodeAdapter::Ptr(new DefaultNodeAdapter(content.get()))));
        }
    }

    object->setStyleSheet(stylesheet);
    object->setObjectName(content->getType().c_str());
    object->setLabel(label);

    return QPixmap::grabWidget(object.get());
}
}

bool BoxManager::typeIsTemplate(const std::string &type)
{
    return type.substr(0,12) == "::template::";
}

std::string BoxManager::getTemplateName(const std::string &type)
{
    assert(typeIsTemplate(type));
    return type.substr(12);
}

bool BoxManager::isValidType(const std::string &type) const
{
    Q_FOREACH(NodeConstructor::Ptr p, available_elements_prototypes) {
        if(p->getType() == type) {
            return true;
        }
    }

    return false;
}

void BoxManager::startPlacingBox(QWidget* parent, const std::string &type, const QPoint& offset)
{
    bool is_template = BoxManager::typeIsTemplate(type);

    Node::Ptr content;
    if(is_template) {
//        content.reset(new Node(""));
    } else {
        Q_FOREACH(NodeConstructor::Ptr p, available_elements_prototypes) {
            if(p->getType() == type) {
                content = p->makePrototypeContent();
            }
        }
    }

    if(content) {
        QDrag* drag = new QDrag(parent);
        QMimeData* mimeData = new QMimeData;

        if(is_template) {
//            mimeData->setData(Template::MIME, "");
        }
        mimeData->setData(Box::MIME, type.c_str());
        mimeData->setProperty("ox", offset.x());
        mimeData->setProperty("oy", offset.y());
        drag->setMimeData(mimeData);

        drag->setPixmap(createPixmap(type, content, style_sheet_));
        drag->setHotSpot(-offset);
        drag->exec();

    } else {
        std::cerr << "unknown box type '" << type << "'!" << std::endl;
    }
}

Node::Ptr BoxManager::makeSingleNode(NodeConstructor::Ptr content, const UUID& uuid)
{
    assert(!BoxManager::typeIsTemplate(content->getType()) && content->getType() != "::group");

    Node::Ptr bo = content->makeContent(uuid);

    return bo;
}

Node::Ptr BoxManager::makeTemplateNode(const UUID& /*uuid*/, const std::string& type)
{
    assert(BoxManager::typeIsTemplate(type) || type == "::group");

    //    csapex::Group::Ptr group(new csapex::Group(type, uuid));

    //    group->setObjectName(uuid.c_str());

    //    return group;
    return Node::Ptr((Node*) NULL);
}

Node::Ptr BoxManager::makeNode(const std::string& target_type, const UUID& uuid)
{
    assert(!uuid.empty());


    if(BoxManager::typeIsTemplate(target_type) || target_type == "::group") {
        return makeTemplateNode(uuid, target_type);
    }

    std::string type = target_type;
    if(type.find_first_of(" ") != type.npos) {
        std::cout << "warning: type '" << type << "' contains spaces, stripping them!" << std::endl;
        while(type.find(" ") != type.npos) {
            type.replace(type.find(" "), 1, "");
        }
    }


    BOOST_FOREACH(NodeConstructor::Ptr p, available_elements_prototypes) {
        if(p->getType() == type) {
            return makeSingleNode(p, uuid);
        }
    }

    std::cout << "warning: cannot make box, type '" << type << "' is unknown, trying different namespace" << std::endl;

    std::string type_wo_ns = UUID::stripNamespace(type);

    BOOST_FOREACH(NodeConstructor::Ptr p, available_elements_prototypes) {
        std::string p_type_wo_ns = UUID::stripNamespace(p->getType());

        if(p_type_wo_ns == type_wo_ns) {
            std::cout << "found a match: '" << type << " == " << p->getType() << std::endl;
            return makeSingleNode(p, uuid);
        }
    }

    std::cerr << "error: cannot make box, type '" << type << "' is unknown\navailable:\n";
    BOOST_FOREACH(NodeConstructor::Ptr p, available_elements_prototypes) {
        std::cerr << p->getType() << '\n';
    }
    std::cerr << std::endl;
    return NodeNullPtr;
}

Box* BoxManager::makeBox(NodePtr node)
{
    BoxedObject::Ptr bo = boost::dynamic_pointer_cast<BoxedObject>(node);
    if(bo) {
        return new Box(bo);
    } else {
        std::string type = node->getType();

        if(node_adapter_builders_.find(type) != node_adapter_builders_.end()) {
            return new Box(node, node_adapter_builders_[type]->build(node));
        } else {
            return new Box(node, NodeAdapter::Ptr(new DefaultNodeAdapter(node.get())));
        }
    }
}

NodeConstructor::Ptr BoxManager::getSelector(const std::string &type)
{
    BOOST_FOREACH(NodeConstructor::Ptr p, available_elements_prototypes) {
        if(p->getType() == type) {
            return p;
        }
    }

    return NodeConstructorNullPtr;
}


void BoxManager::setContainer(QWidget *c)
{
    container_ = c;
}

QWidget* BoxManager::container()
{
    return container_;
}
