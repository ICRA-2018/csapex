/// HEADER
#include <csapex/design_board.h>

/// PROJECT
#include "ui_design_board.h"
#include <csapex/connector.h>
#include <csapex/selector_proxy.h>
#include <csapex/box.h>
#include <csapex/command_add_box.h>
#include <csapex/box_manager.h>

/// SYSTEM
#include <boost/foreach.hpp>
#include <QResizeEvent>
#include <QMenu>
#include <iostream>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QScrollArea>
#include <QScrollBar>

using namespace csapex;

DesignBoard::DesignBoard(QWidget* parent)
    : QWidget(parent), ui(new Ui::DesignBoard), space_(false), drag_(false)
{
    ui->setupUi(this);

    overlay = new Overlay(this);

    installEventFilter(this);

    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    BoxManager::instance().setContainer(this);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
}

DesignBoard::~DesignBoard()
{}

void DesignBoard::updateCursor()
{
    if(space_) {
        if(drag_) {
            setCursor(Qt::ClosedHandCursor);
        } else {
            setCursor(Qt::OpenHandCursor);
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void DesignBoard::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    updateCursor();
}

void DesignBoard::findMinSize(Box* box)
{
    QSize minimum = minimumSize();

    minimum.setWidth(std::max(minimum.width(), box->pos().x() + box->width()));
    minimum.setHeight(std::max(minimum.height(), box->pos().y() + box->height()));

    int movex = box->x() < 0 ? -box->x() : 0;
    int movey = box->y() < 0 ? -box->y() : 0;

    if(movex != 0 || movey != 0) {
        BOOST_FOREACH(csapex::Box* b, findChildren<csapex::Box*>()) {
            if(b != box) {
                b->move(b->x() + movex, b->y() + movey);
            }
        }
        minimum.setWidth(minimum.width() + movex);
        minimum.setHeight(minimum.height() + movey);
    }

    setMinimumSize(minimum);
}

void DesignBoard::keyPressEvent(QKeyEvent* e)
{
    if(!BoxManager::instance().keyPressEventHandler(e)) {
        return;
    }
    if(!overlay->keyPressEventHandler(e)) {
        return;
    }

    if(e->key() == Qt::Key_Space) {
        space_ = true;
    }
}

void DesignBoard::keyReleaseEvent(QKeyEvent* e)
{
    if(!BoxManager::instance().keyReleaseEventHandler(e)) {
        return;
    }
    if(!overlay->keyReleaseEventHandler(e)) {
        return;
    }

    if(!e->isAutoRepeat() && e->key() == Qt::Key_Space) {
        space_ = false;
        drag_ = false;
    }
}

void DesignBoard::mousePressEvent(QMouseEvent* e)
{
    if(!drag_ || !space_) {
        if(!BoxManager::instance().mousePressEventHandler(e)) {
            return;
        }
        if(!overlay->mousePressEventHandler(e)) {
            return;
        }
    }

    if(e->button() == Qt::LeftButton) {
        drag_ = true;
        drag_start_pos_ = e->globalPos();
        updateCursor();
    }
}

void DesignBoard::mouseReleaseEvent(QMouseEvent* e)
{
    if(e->button() == Qt::LeftButton) {
        drag_ = false;

        overlay->setSelectionRectangle(QPoint(),QPoint());
        QRect selection(mapFromGlobal(drag_start_pos_), mapFromGlobal(e->globalPos()));
        if(std::abs(selection.width()) > 5 && std::abs(selection.height()) > 5) {
            BoxManager::instance().deselectBoxes();

            BOOST_FOREACH(csapex::Box* box, findChildren<csapex::Box*>()) {
                if(selection.contains(box->geometry())) {
                    BoxManager::instance().selectBox(box, true);
                }
            }

            return;
        }
    }


    if(!BoxManager::instance().mouseReleaseEventHandler(e)) {
        return;
    }
    if(!overlay->mouseReleaseEventHandler(e)) {
        return;
    }
    updateCursor();
}

void DesignBoard::mouseMoveEvent(QMouseEvent* e)
{
    if(drag_) {
        if( space_) {
            QSize minimum = minimumSize();
            if(minimum.width() < size().width()) {
                minimum.setWidth(size().width());
            }
            if(minimum.height() < size().height()) {
                minimum.setHeight(size().height());
            }

            setMinimumSize(minimum);

            updateCursor();
            QPoint delta = e->globalPos() - drag_start_pos_;
            drag_start_pos_ = e->globalPos();

            QScrollArea* parent_scroll = NULL;
            QWidget* tmp = parentWidget();
            while(tmp != NULL) {
                parent_scroll = dynamic_cast<QScrollArea*>(tmp);
                if(parent_scroll) {
                    break;
                }
                tmp = tmp->parentWidget();
            }

            if(parent_scroll) {
                int sbh = parent_scroll->horizontalScrollBar()->value();
                int sbv = parent_scroll->verticalScrollBar()->value();

                parent_scroll->horizontalScrollBar()->setValue(sbh - delta.x());
                parent_scroll->verticalScrollBar()->setValue(sbv - delta.y());

                int sbh_after = parent_scroll->horizontalScrollBar()->value();
                int sbv_after = parent_scroll->verticalScrollBar()->value();

                int dx = sbh - delta.x() - sbh_after;
                int dy = sbv - delta.y() - sbv_after;

                if(dx != 0 || dy != 0) {
                    QSize minimum = minimumSize();

                    minimum.setWidth(minimum.width() + std::abs(dx));
                    minimum.setHeight(minimum.height() + std::abs(dy));

                    int movex = dx < 0 ? -dx : 0;
                    int movey = dy < 0 ? -dy : 0;

                    if(movex != 0 || movey != 0) {
                        BOOST_FOREACH(csapex::Box* box, findChildren<csapex::Box*>()) {
                            box->move(box->x() + movex, box->y() + movey);
                        }
                    }

                    setMinimumSize(minimum);
                }
            }
        } else {
            overlay->setSelectionRectangle(overlay->mapFromGlobal(drag_start_pos_), overlay->mapFromGlobal(e->globalPos()));
            overlay->repaint();
        }
    } else if(!overlay->mouseMoveEventHandler(e)) {
        return;
    }
    if(!BoxManager::instance().mouseMoveEventHandler(e)) {
        return;
    }
    if(!overlay->mouseMoveEventHandler(e)) {
        return;
    }
}

bool DesignBoard::eventFilter(QObject* o, QEvent* e)
{
    if(e->type() == QEvent::ChildPolished) {
        QChildEvent* ch = dynamic_cast<QChildEvent*>(e);
        QObject* child = ch->child();
        Box* box = dynamic_cast<Box*>(child);
        if(box) {
            findMinSize(box);

            QObject::connect(box, SIGNAL(moved(Box*, int, int)), this, SLOT(findMinSize(Box*)));
            QObject::connect(box, SIGNAL(moved(Box*, int, int)), overlay, SLOT(invalidateSchema()));
            QObject::connect(box, SIGNAL(moved(Box*, int, int)), &BoxManager::instance(), SLOT(boxMoved(Box*, int, int)));
            QObject::connect(box, SIGNAL(changed(Box*)), overlay, SLOT(invalidateSchema()));
            QObject::connect(box, SIGNAL(clicked(Box*)), &BoxManager::instance(), SLOT(toggleBoxSelection(Box*)));
            QObject::connect(box, SIGNAL(connectorCreated(Connector*)), overlay, SLOT(connectorAdded(Connector*)));
            QObject::connect(box, SIGNAL(connectionFormed(ConnectorOut*,ConnectorIn*)), overlay, SLOT(addConnection(ConnectorOut*,ConnectorIn*)));
            QObject::connect(box, SIGNAL(connectionDestroyed(ConnectorOut*,ConnectorIn*)), overlay, SLOT(removeConnection(ConnectorOut*,ConnectorIn*)));
            QObject::connect(box, SIGNAL(connectorEnabled(Connector*)), overlay, SLOT(connectorEnabled(Connector*)));
            QObject::connect(box, SIGNAL(connectorDisabled(Connector*)), overlay, SLOT(connectorDisabled(Connector*)));

            QObject::connect(box, SIGNAL(messageSent(ConnectorOut*)), overlay, SLOT(showPublisherSignal(ConnectorOut*)));
            QObject::connect(box, SIGNAL(messageArrived(ConnectorIn*)), overlay, SLOT(showPublisherSignal(ConnectorIn*)));
            QObject::connect(box, SIGNAL(connectionStart()), overlay, SLOT(deleteTemporaryConnections()));
            QObject::connect(box, SIGNAL(connectionInProgress(Connector*,Connector*)), overlay, SLOT(addTemporaryConnection(Connector*,Connector*)));
            QObject::connect(box, SIGNAL(connectionDone()), overlay, SLOT(deleteTemporaryConnectionsAndRepaint()));

            box->registered();
        }

        overlay->raise();
    }

    return false;
}


void DesignBoard::showContextMenu(const QPoint& pos)
{
    QPoint globalPos = mapToGlobal(pos);

    QMenu menu;
    BoxManager::instance().fill(&menu);

    QAction* selectedItem = menu.exec(globalPos);

    if(selectedItem) {
        std::string selected = selectedItem->data().toString().toUtf8().constData();
        BoxManager::instance().startPlacingBox(selected);
    }
}

void DesignBoard::resizeEvent(QResizeEvent* e)
{
    overlay->resize(e->size());
}

void DesignBoard::dragEnterEvent(QDragEnterEvent* e)
{
    if(e->mimeData()->text() == Box::MIME) {
        e->acceptProposedAction();
    }
    if(e->mimeData()->text() == Box::MIME_MOVE) {
        e->acceptProposedAction();
    }
    if(e->mimeData()->text() == Connector::MIME_CREATE) {
        e->acceptProposedAction();
    }
    if(e->mimeData()->text() == Connector::MIME_MOVE) {
        e->acceptProposedAction();
    }
}

void DesignBoard::dragMoveEvent(QDragMoveEvent* e)
{
    if(e->mimeData()->text() == Connector::MIME_CREATE) {
        Connector* c = dynamic_cast<Connector*>(e->mimeData()->parent());
        overlay->deleteTemporaryConnections();
        overlay->addTemporaryConnection(c, e->pos());
    }

    if(e->mimeData()->text() == Connector::MIME_MOVE) {
        Connector* c = dynamic_cast<Connector*>(e->mimeData()->parent());
        overlay->deleteTemporaryConnections();

        ConnectorOut* out = dynamic_cast<ConnectorOut*> (c);
        if(out) {
            for(ConnectorOut::TargetIterator it = out->beginTargets(); it != out->endTargets(); ++it) {
                overlay->addTemporaryConnection(*it, e->pos());
            }
        } else {
            ConnectorIn* in = dynamic_cast<ConnectorIn*> (c);
            overlay->addTemporaryConnection(in->getConnected(), e->pos());
        }
    }

    if(e->mimeData()->text() == Box::MIME_MOVE) {
        Box* box = dynamic_cast<Box*>(e->mimeData()->parent());
        Box::MoveOffset* offset = dynamic_cast<Box::MoveOffset*>(e->mimeData()->userData(0));
        box->move(e->pos() + offset->value);

        overlay->repaint();
    }
}

void DesignBoard::dropEvent(QDropEvent* e)
{
    if(e->mimeData()->text() == Box::MIME) {
        SelectorProxy* selector = dynamic_cast<SelectorProxy*>(e->mimeData()->parent());

        if(!selector) {
            return;
        }

        e->setDropAction(Qt::CopyAction);
        e->accept();

        QPoint offset (e->mimeData()->property("ox").toInt(), e->mimeData()->property("oy").toInt());
        QPoint pos = e->pos() + offset;

        Command::Ptr add_box(new command::AddBox(selector, this, pos));
        BoxManager::instance().execute(add_box);
    }

    if(e->mimeData()->text() == Connector::MIME_CREATE) {
        e->ignore();
    }
    if(e->mimeData()->text() == Connector::MIME_MOVE) {
        e->ignore();
    }
}

Overlay* DesignBoard::getOverlay()
{
    return overlay;
}