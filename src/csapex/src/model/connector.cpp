/// HEADER
#include <csapex/model/connector.h>

/// COMPONENT
#include <csapex/view/design_board.h>
#include <csapex/model/box.h>
#include <csapex/model/boxed_object.h>
#include <csapex/command/dispatcher.h>
#include <csapex/command/add_connection.h>
#include <csapex/command/move_connection.h>

/// SYSTEM
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <QDragEnterEvent>
#include <QPainter>

using namespace csapex;

const QString Connector::MIME_CREATE = "csapex/connector/create";
const QString Connector::MIME_MOVE = "csapex/connector/move";

const std::string Connector::namespace_separator = ":|:";


std::string Connector::makeUUID(const std::string& box_uuid, int type, int sub_id) {
    std::stringstream ss;
    ss << box_uuid << namespace_separator << (type > 0 ? "in" : (type == 0 ? "out" : "~")) << "_" << sub_id;
    return ss.str();
}

Connector::Connector(Box* parent, const std::string& uuid)
    : parent_widget(parent), designer(NULL), buttons_down_(0), uuid_(uuid), minimized_(false)
{
    init(parent);
}

Connector::Connector(Box* parent, int sub_id, int type)
    : parent_widget(parent), designer(NULL), buttons_down_(0), uuid_(makeUUID(parent->UUID(), type, sub_id)), minimized_(false)
{
    init(parent);
}

void Connector::init(Box* parent)
{
    setBox(parent);

    findParents();
    setFocusPolicy(Qt::NoFocus);
    setAcceptDrops(true);

    setContextMenuPolicy(Qt::PreventContextMenu);
    setType(ConnectionType::makeDefault());

    setMinimizedSize(minimized_);

    setMouseTracking(true);

    setToolTip(uuid_.c_str());
}


Connector::~Connector()
{
}

void Connector::errorEvent(bool error, ErrorLevel level)
{
    box_->getContent()->setError(error, "Connector Error", level);
}

bool Connector::isForwarding() const
{
    return false;
}

std::string Connector::UUID()
{
    return uuid_;
}

bool Connector::tryConnect(QObject* other_side)
{
    Connector* c = dynamic_cast<Connector*>(other_side);
    if(c) {
        return tryConnect(c);
    }
    return false;
}

void Connector::removeConnection(QObject* other_side)
{
    Connector* c = dynamic_cast<Connector*>(other_side);
    if(c) {
        removeConnection(c);
    }
}

void Connector::validateConnections()
{

}

void Connector::removeAllConnectionsUndoable()
{
    if(isConnected()) {
        Command::Ptr cmd = removeAllConnectionsCmd();
        CommandDispatcher::execute(cmd);
    }
}

void Connector::disable()
{
    setEnabled(false);
    Q_EMIT disabled(this);
}

void Connector::enable()
{
    setEnabled(true);
    Q_EMIT enabled(this);
}

void Connector::findParents()
{
    QWidget* tmp = this;
    while(tmp != NULL) {
        if(dynamic_cast<csapex::Box*>(tmp)) {
            box_ = dynamic_cast<csapex::Box*>(tmp);
        } else if(dynamic_cast<csapex::DesignBoard*>(tmp)) {
            designer = dynamic_cast<csapex::DesignBoard*>(tmp);
        }
        tmp = tmp->parentWidget();
    }
}

bool Connector::canConnectTo(Connector* other_side) const
{
    if(other_side == this) {
        return false;
    }

    bool in_out = (canOutput() && other_side->canInput()) || (canInput() && other_side->canOutput());
    bool compability = getType()->canConnectTo(other_side->getType());

    return in_out && compability;
}

void Connector::dragEnterEvent(QDragEnterEvent* e)
{
    if(e->mimeData()->hasFormat(Connector::MIME_CREATE)) {
        Connector* from = dynamic_cast<Connector*>(e->mimeData()->parent());
        if(from->canConnectTo(this)) {
            if(canConnectTo(from)) {
                e->acceptProposedAction();
            }
        }
    } else if(e->mimeData()->hasFormat(Connector::MIME_MOVE)) {
        Connector* from = dynamic_cast<Connector*>(e->mimeData()->parent());

        if(from->targetsCanConnectTo(this)) {
            e->acceptProposedAction();
        }
    }
}

void Connector::dragMoveEvent(QDragMoveEvent* e)
{
    Q_EMIT(connectionStart());
    if(e->mimeData()->hasFormat(Connector::MIME_CREATE)) {
        Connector* from = dynamic_cast<Connector*>(e->mimeData()->parent());
        Q_EMIT(connectionInProgress(this, from));

    } else if(e->mimeData()->hasFormat(Connector::MIME_MOVE)) {
        Connector* from = dynamic_cast<Connector*>(e->mimeData()->parent());

        from->connectionMovePreview(this);
    }
}

void Connector::dropEvent(QDropEvent* e)
{
    if(e->mimeData()->hasFormat(Connector::MIME_CREATE)) {
        Connector* from = dynamic_cast<Connector*>(e->mimeData()->parent());

        if(from && from != this) {
            Command::Ptr cmd(new command::AddConnection(UUID(), from->UUID()));
            CommandDispatcher::execute(cmd);
        }
    } else if(e->mimeData()->hasFormat(Connector::MIME_MOVE)) {
        Connector* from = dynamic_cast<Connector*>(e->mimeData()->parent());

        if(from && from != this) {
            Command::Ptr cmd(new command::MoveConnection(from, this));
            CommandDispatcher::execute(cmd);
            e->setDropAction(Qt::MoveAction);
        }
    }
}

void Connector::mousePressEvent(QMouseEvent* e)
{
    buttons_down_ = e->buttons();
}

bool Connector::shouldCreate(bool left, bool)
{
    bool full_input = canInput() && isConnected();
    return left && !full_input;
}

bool Connector::shouldMove(bool left, bool right)
{
    bool full_input = canInput() && isConnected();
    return (right && isConnected()) || (left && full_input);
}

void Connector::mouseMoveEvent(QMouseEvent* e)
{
    if(buttons_down_ == Qt::NoButton) {
        return;
    }

    bool left = (buttons_down_ & Qt::LeftButton) != 0;
    bool right = (buttons_down_ & Qt::RightButton) != 0;

    bool create = shouldCreate(left, right);
    bool move = shouldMove(left, right);

    if(create || move) {
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;

        if(move) {
            mimeData->setData(Connector::MIME_MOVE, QByteArray());
            mimeData->setParent(this);
            drag->setMimeData(mimeData);

            disable();

            drag->exec();

            enable();

        } else {
            mimeData->setData(Connector::MIME_CREATE, QByteArray());
            mimeData->setParent(this);
            drag->setMimeData(mimeData);

            drag->exec();
        }

        e->accept();

        Q_EMIT connectionDone();
        buttons_down_ = Qt::NoButton;
    }
    e->accept();
}

void Connector::mouseReleaseEvent(QMouseEvent* e)
{
    buttons_down_ = e->buttons();

    if(e->button() == Qt::MiddleButton) {
        removeAllConnectionsUndoable();
    } else if(e->button() == Qt::RightButton) {
        removeAllConnectionsUndoable();
    }

    e->accept();
}

QPoint Connector::topLeft()
{
    if(box_ == NULL) {
        findParents();
    }

    return box_->geometry().topLeft() + pos();
}

QPoint Connector::centerPoint()
{
    return topLeft() + 0.5 * (geometry().bottomRight() - geometry().topLeft());
}

std::string Connector::getLabel() const
{
    return label_;
}

void Connector::setLabel(const std::string &label)
{
    label_ = label;
}

void Connector::setType(ConnectionType::ConstPtr type)
{
    type_ = type;

    validateConnections();
}

ConnectionType::ConstPtr Connector::getType() const
{
    return type_;
}

void Connector::setMinimizedSize(bool mini)
{
    minimized_ = mini;

    if(mini) {
        setFixedSize(16,8);
    } else {
        setFixedSize(24,16);
    }
}

bool Connector::isMinimizedSize() const
{
    return minimized_;
}

void Connector::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setBrush(Qt::black);
    p.setOpacity(0.35);
    p.drawEllipse(contentsRect().center(), 2, 2);
}
