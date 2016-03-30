/// HEADER
#include <csapex/view/widgets/port.h>

/// COMPONENT
#include <csapex/model/node.h>
#include <csapex/command/dispatcher.h>
#include <csapex/command/add_connection.h>
#include <csapex/command/move_connection.h>
#include <csapex/command/command_factory.h>
#include <csapex/msg/input.h>
#include <csapex/msg/static_output.h>
#include <csapex/view/designer/graph_view.h>
#include <csapex/view/widgets/message_preview_widget.h>
#include <csapex/view/designer/designer_scene.h>

/// SYSTEM
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <QDrag>
#include <QWidget>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QHelpEvent>
#include <QEvent>

using namespace csapex;

Port::Port(QWidget *parent)
    : QFrame(parent),
      refresh_style_sheet_(false), minimized_(false), flipped_(false), buttons_down_(0)
{
    setFlipped(flipped_);

    setFocusPolicy(Qt::NoFocus);
    setAcceptDrops(true);

    setContextMenuPolicy(Qt::PreventContextMenu);

    setMinimizedSize(minimized_);

    setEnabled(true);
}

Port::Port(ConnectableWeakPtr adaptee, QWidget *parent)
    : Port(parent)
{
    adaptee_ = adaptee;
    ConnectablePtr adaptee_ptr = adaptee_.lock();
    if(adaptee_ptr) {
        createToolTip();

        connections_.push_back(adaptee_ptr->enabled_changed.connect([this](bool e) { setEnabledFlag(e); }));
        connections_.push_back(adaptee_ptr->connectableError.connect([this](bool error,std::string msg,int level) { setError(error, msg, level); }));

        if(adaptee_ptr->isDynamic()) {
            setProperty("dynamic", true);
        }

    } else {
        std::cerr << "creating empty port!" << std::endl;
    }
}


Port::~Port()
{
    for(auto& c : connections_) {
        c.disconnect();
    }
}

bool Port::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        ConnectablePtr adaptee = adaptee_.lock();
        if(!adaptee) {
            return false;
        }

        createToolTip();
    }

    return QWidget::event(e);
}

bool Port::canOutput() const
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return false;
    }
    return adaptee->canOutput();
}

bool Port::canInput() const
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return false;
    }
    return adaptee->canInput();
}

bool Port::isOutput() const
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return false;
    }
    return adaptee->isOutput();
}

bool Port::isInput() const
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return false;
    }
    return adaptee->isInput();
}

ConnectableWeakPtr Port::getAdaptee() const
{
    return adaptee_;
}


void Port::paintEvent(QPaintEvent *e)
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return;
    }
    bool opt = dynamic_cast<Input*>(adaptee.get()) && dynamic_cast<Input*>(adaptee.get())->isOptional();
    setProperty("unconnected", isInput() && !opt && !adaptee->isConnected());
    setProperty("optional", opt);

    if(refresh_style_sheet_) {
        refresh_style_sheet_ = false;
        setStyleSheet(styleSheet());
    }
    QFrame::paintEvent(e);
}

void Port::refreshStylesheet()
{
    refresh_style_sheet_ = true;
}

void Port::setError(bool error, const std::string& msg)
{
    setError(error, msg, static_cast<int>(ErrorState::ErrorLevel::ERROR));
}

void Port::setError(bool error, const std::string& /*msg*/, int /*level*/)
{
    setProperty("error", error);
    refreshStylesheet();
}

void Port::setMinimizedSize(bool mini)
{
    minimized_ = mini;

    if(mini) {
        setFixedSize(8,8);
    } else {
        setFixedSize(16,16);
    }
}

bool Port::isMinimizedSize() const
{
    return minimized_;
}

void Port::setFlipped(bool flipped)
{
    flipped_ = flipped;
}

bool Port::isFlipped() const
{
    return flipped_;
}

void Port::setEnabledFlag(bool enabled)
{
    setPortProperty("enabled", enabled);
    setPortProperty("disabled", !enabled);
    setEnabled(true);
    refreshStylesheet();
}

void Port::setPortProperty(const std::string& name, bool b)
{
    setProperty(name.c_str(), b);
}

void Port::createToolTip()
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return;
    }

    std::stringstream tooltip;
    tooltip << "UUID: " << adaptee->getUUID();
    tooltip << ", Type: " << adaptee->getType()->descriptiveName();
    tooltip << ", Connections: " << adaptee->getConnections().size();
    tooltip << ", Messages: " << adaptee->getCount();
    tooltip << ", Enabled: " << adaptee->isEnabled();
    tooltip << ", #: " << adaptee->sequenceNumber();

    Output* o = dynamic_cast<Output*>(adaptee.get());
    if(o) {
        tooltip << ", state: ";
        switch(o->getState()) {
        case Output::State::ACTIVE:
            tooltip << "ACTIVE";
            break;
        case Output::State::IDLE:
            tooltip << "IDLE";
            break;
        default:
            tooltip << "UNKNOWN";
        }
    }
    setToolTip(tooltip.str().c_str());
}

void Port::startDrag()
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return;
    }

    bool left = (buttons_down_ & Qt::LeftButton) != 0;
    bool right = (buttons_down_ & Qt::RightButton) != 0;

    bool create = adaptee->shouldCreate(left, right);
    bool move = adaptee->shouldMove(left, right);

    adaptee->connectionStart(adaptee.get());

    if(create || move) {
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;

        if(move) {
            mimeData->setData(QString::fromStdString(Connectable::MIME_MOVE_CONNECTIONS), QByteArray());
            mimeData->setProperty("connectable", qVariantFromValue(static_cast<void*> (adaptee.get())));

            drag->setMimeData(mimeData);

            drag->exec();

        } else {
            mimeData->setData(QString::fromStdString(Connectable::MIME_CREATE_CONNECTION), QByteArray());
            mimeData->setProperty("connectable", qVariantFromValue(static_cast<void*> (adaptee.get())));

            drag->setMimeData(mimeData);

            drag->exec();
        }

        adaptee->connection_added_to(adaptee.get());
        buttons_down_ = Qt::NoButton;
    }
}

void Port::mouseMoveEvent(QMouseEvent* e)
{
    if(buttons_down_ == Qt::NoButton) {
        return;
    }

    startDrag();

    e->accept();
}

void Port::mouseReleaseEvent(QMouseEvent* e)
{
    startDrag();

    buttons_down_ = e->buttons();

    if(e->button() == Qt::MiddleButton) {
        Q_EMIT removeConnectionsRequest();
    }

    e->accept();
}

void Port::dragEnterEvent(QDragEnterEvent* e)
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return;
    }
    if(e->mimeData()->hasFormat(QString::fromStdString(Connectable::MIME_CREATE_CONNECTION))) {
        Connectable* from = static_cast<Connectable*>(e->mimeData()->property("connectable").value<void*>());
        if(from == adaptee.get()) {
            return;
        }

        if(from->canConnectTo(adaptee.get(), false)) {
            if(adaptee->canConnectTo(from, false)) {
                e->acceptProposedAction();
                Q_EMIT(adaptee->connectionInProgress(adaptee.get(), from));
            }
        }
    } else if(e->mimeData()->hasFormat(QString::fromStdString(Connectable::MIME_MOVE_CONNECTIONS))) {
        Connectable* original = static_cast<Connectable*>(e->mimeData()->property("connectable").value<void*>());

        if(original->targetsCanBeMovedTo(adaptee.get())) {
            e->acceptProposedAction();
        }
    }
}

void Port::dragMoveEvent(QDragMoveEvent* e)
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return;
    }
    if(e->mimeData()->hasFormat(QString::fromStdString(Connectable::MIME_CREATE_CONNECTION))) {
        e->acceptProposedAction();

    } else if(e->mimeData()->hasFormat(QString::fromStdString(Connectable::MIME_MOVE_CONNECTIONS))) {
        Connectable* from = static_cast<Connectable*>(e->mimeData()->property("connectable").value<void*>());

        from->connectionMovePreview(adaptee.get());

        e->acceptProposedAction();
    }
}

void Port::dropEvent(QDropEvent* e)
{
    ConnectablePtr adaptee = adaptee_.lock();
    if(!adaptee) {
        return;
    }
    if(e->mimeData()->hasFormat(QString::fromStdString(Connectable::MIME_CREATE_CONNECTION))) {
        Connectable* from = static_cast<Connectable*>(e->mimeData()->property("connectable").value<void*>());
        if(from && from != adaptee.get()) {
            addConnectionRequest(from);
        }
    } else if(e->mimeData()->hasFormat(QString::fromStdString(Connectable::MIME_MOVE_CONNECTIONS))) {
        Connectable* from = static_cast<Connectable*>(e->mimeData()->property("connectable").value<void*>());
        if(from) {
            moveConnectionRequest(from);
            e->setDropAction(Qt::MoveAction);
        }
    }
}

void Port::enterEvent(QEvent */*e*/)
{
    Q_EMIT mouseOver(this);
}

void Port::leaveEvent(QEvent */*e*/)
{
    Q_EMIT mouseOut(this);
}

void Port::mousePressEvent(QMouseEvent* e)
{
    buttons_down_ = e->buttons();
}

/// MOC
#include "../../../include/csapex/view/widgets/moc_port.cpp"
