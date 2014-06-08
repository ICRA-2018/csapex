/// HEADER
#include <csapex/view/designer_scene.h>

/// COMPONENT
#include <csapex/model/connectable.h>
#include <csapex/model/graph.h>
#include <csapex/view/widget_controller.h>
#include <csapex/view/port.h>
#include <csapex/model/node.h>
#include <csapex/view/box.h>
#include <csapex/model/connector_in.h>
#include <csapex/model/connector_out.h>
#include <csapex/core/settings.h>
#include <csapex/command/dispatcher.h>
#include <csapex/command/add_fulcrum.h>
#include <csapex/command/move_fulcrum.h>
#include <csapex/command/modify_fulcrum.h>
#include <csapex/view/fulcrum_widget.h>
#include <csapex/utility/movable_graphics_proxy_widget.h>

/// SYSTEM
#include <QtGui>
#include <QtOpenGL>
#include <GL/glu.h>
#include <assert.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

using namespace csapex;

const float DesignerScene::ARROW_LENGTH = 10.f;

namespace {
QRgb id2rgb(int id, int subsection)
{
    assert(id < 0xFFFF);
    assert(subsection < 0xFF);
    int raw = (id & 0xFFFF) | ((subsection & 0xFF) << 16);

    return qRgb(qRed(raw), qGreen(raw), qBlue(raw));
}

std::pair<int,int> rgb2id(QRgb rgb)
{
    int raw = (rgb & 0xFFFFFF);
    if(raw >= 0xFFFFFF) {
        return std::make_pair(-1, -1);
    }

    int id = raw & 0xFFFF;
    int subsection = (raw >> 16) & 0xFF;

    return std::make_pair(id, subsection);
}

QWidget* topLevelParentWidget (QWidget* widget)
{
    while (widget -> parentWidget()) widget = widget -> parentWidget() ;
    return widget ;
}
}



DesignerScene::DesignerScene(GraphPtr graph, CommandDispatcher *dispatcher, WidgetControllerPtr widget_ctrl)
    : graph_(graph), dispatcher_(dispatcher), widget_ctrl_(widget_ctrl),
      draw_grid_(false), draw_schema_(false), scale_(1.0),
      schema_dirty_(false)
{
    background_ = QPixmap::fromImage(QImage(":/background.png"));

    activity_marker_min_width_ = 3;
    activity_marker_max_width_ = 8;
    activity_marker_min_opacity_ = 50;
    activity_marker_max_opacity_ = 90;

    connector_radius_ = 7;

    output_color_ = QColor(0xFF, 0xCC, 0x00);
    input_color_ = QColor(0xFF, 0x00, 0xCC);

    QObject::connect(graph_.get(), SIGNAL(connectionAdded(Connection*)), this, SLOT(connectionAdded(Connection*)));
    QObject::connect(graph_.get(), SIGNAL(connectionDeleted(Connection*)), this, SLOT(connectionDeleted(Connection*)));
}

DesignerScene::~DesignerScene()
{

}

void DesignerScene::enableGrid(bool draw)
{
    if(draw != draw_grid_) {
        draw_grid_ = draw;

        update();
    }
}

void DesignerScene::enableSchema(bool draw)
{
    if(draw != draw_schema_) {
        draw_schema_ = draw;
        update();
    }
}

void DesignerScene::setScale(double scale)
{
    scale_ = scale;
}

void DesignerScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    if(draw_grid_) {
        bool spt = painter->renderHints() & QPainter::SmoothPixmapTransform;
        painter->setRenderHint(QPainter::SmoothPixmapTransform, scale_ < 1.0);

        qreal o = painter->opacity();
        static const double max_distance = 0.4;
        if(scale_ < 1.0) {
            qreal op = (scale_ - max_distance) / max_distance;
            painter->setOpacity(op);
        }

        painter->drawTiledPixmap(sceneRect(), background_);

        painter->setOpacity(o);

        painter->setRenderHint(QPainter::SmoothPixmapTransform, spt);
    }
}

void DesignerScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);

    QSize scene_size(sceneRect().width(), sceneRect().height());
    if(schematics.isNull() || schematics.size() != scene_size ) {
        schematics = QImage(scene_size, QImage::Format_RGB888);
        schema_dirty_ = true;
    }

    QPainter ps(&schematics);
    ps.setWindow(sceneRect().toRect());
    schematics_painter = &ps;

    if(schema_dirty_) {
        schematics_painter->fillRect(sceneRect(), -1);
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(Qt::black, 3));

    if(!temp_.empty()) {
        foreach(const TempConnection& temp, temp_) {

            ccs = CurrentConnectionState();
            Port* fromp = widget_ctrl_->getPort(temp.from);
            bool flipped = fromp->isFlipped();

            if(temp.from->isInput()) {
                drawConnection(painter, temp.to, topLevelParentWidget(fromp)->pos() + fromp->centerPoint(), -1,
                               flipped ? Fulcrum::IN : Fulcrum::OUT,
                               Fulcrum::HANDLE);
            } else {
                drawConnection(painter, topLevelParentWidget(fromp)->pos() + fromp->centerPoint(), temp.to, -1,
                               flipped ? Fulcrum::IN : Fulcrum::OUT,
                               Fulcrum::HANDLE);
            }
        }
    }

    Q_FOREACH(Connection::Ptr connection, graph_->connections_) {
        drawConnection(painter, *connection);
    }

    foreach (Node::Ptr node, graph_->nodes_) {

        NodeBox* box = widget_ctrl_->getBox(node->getUUID());
        if(!box) {
            continue;
        }

        if(node->isError()) {
            QRectF rect(box->pos() + QPoint(0, box->height() + 8), QSize(box->width(), 64));

            QFont font;
            font.setPixelSize(8);
            painter->setFont(font);
            painter->setPen(node->errorLevel() == Node::EL_ERROR ? Qt::red : QColor(0xCC,0x99,0x00));

            QTextOption opt(Qt::AlignTop | Qt::AlignHCenter);
            painter->drawText(rect, node->errorMessage().c_str(), opt);
        }

        for(int id = 0; id < node->countInputs(); ++id) {
            Port* p = widget_ctrl_->getPort(node->getInput(id));
            if(p) {
                drawPort(painter, box, p);
            }
        }
        for(int id = 0; id < node->countOutputs(); ++id) {
            ConnectorOut* o = node->getOutput(id);
            assert(o->guard_ == 0xDEADBEEF);
            Port* p = widget_ctrl_->getPort(o);
            if(p) {
                assert(p->guard_ == 0xDEADBEEF);
                drawPort(painter, box, p);
            }
        }
    }

    if(draw_schema_){
        painter->setOpacity(0.35);
        painter->drawImage(sceneRect().topLeft(), schematics);
    }

    schema_dirty_ = false;

    schematics_painter = NULL;
}

void DesignerScene::drawItems(QPainter *painter, int numItems,
                              QGraphicsItem *items[],
                              const QStyleOptionGraphicsItem options[],
                              QWidget *widget)
{
    QGraphicsScene::drawItems(painter, numItems, items, options, widget);
}

void DesignerScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if(e->button() == Qt::RightButton && highlight_connection_id_ != -1) {
        e->accept();
        showConnectionContextMenu();
        return;

    } else if(e->button() == Qt::MiddleButton && highlight_connection_id_ != -1) {
        dispatcher_->execute(graph_->deleteConnectionById(highlight_connection_id_));
        return;
    }

    QGraphicsScene::mousePressEvent(e);

    if(!e->isAccepted() && e->button() == Qt::LeftButton) {
        if(highlight_connection_id_ != -1) {
            QPoint pos = e->scenePos().toPoint();
            dispatcher_->execute(Command::Ptr(new command::AddFulcrum(highlight_connection_id_, highlight_connection_sub_id_, pos, 0)));
            e->accept();

            // allow moving the fulcrum directly
            QGraphicsScene::mousePressEvent(e);
        }
    }
}

void DesignerScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mouseReleaseEvent(e);
}

void DesignerScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mouseMoveEvent(e);

    QPoint pos = (e->scenePos() - sceneRect().topLeft()).toPoint();
    if(!schematics.rect().contains(pos)) {
        return;
    }

    std::pair<int, int> data = rgb2id(schematics.pixel(pos.x(),pos.y()));

    if(data.first != highlight_connection_id_) {
        highlight_connection_id_ = data.first;
        highlight_connection_sub_id_ = data.second;
        update();
    }
}

QPen DesignerScene::makeSelectedLinePen(const QPointF& from, const QPointF& to)
{
    QLinearGradient lg(from, to);
    if(ccs.error) {
        lg.setColorAt(0, Qt::darkRed);
        lg.setColorAt(1, Qt::red);
    } else {
        lg.setColorAt(0, output_color_.darker());
        lg.setColorAt(1, input_color_.darker());
    }

    return QPen(QBrush(lg), ccs.r * 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

QPen DesignerScene::makeLinePen(const QPointF& from, const QPointF& to)
{
    QLinearGradient lg(from, to);
    if(ccs.error) {
        lg.setColorAt(0,Qt::darkRed);
        lg.setColorAt(1,Qt::red);

    } else if(ccs.disabled) {
        lg.setColorAt(0,Qt::darkGray);
        lg.setColorAt(1,Qt::gray);
    } else if(ccs.async) {
        lg.setColorAt(0, QColor(0xFF, 0xCC, 0x00));
        lg.setColorAt(1, QColor(0xFF, 0xCC, 0x00));
    } else {
        QColor a = output_color_;
        QColor b = input_color_;
        lg.setColorAt(0, a);
        lg.setColorAt(1, b);
    }

    return QPen(QBrush(lg), ccs.r * 0.75, /*from.x() > to.x() ? Qt::DotLine : Qt::SolidLine*/ Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}


void DesignerScene::connectionAdded(Connection* c)
{
    QObject::connect(c, SIGNAL(fulcrum_added(Fulcrum *)), this, SLOT(fulcrumAdded(Fulcrum *)));
    QObject::connect(c, SIGNAL(fulcrum_deleted(Fulcrum*)), this, SLOT(fulcrumDeleted(Fulcrum*)), Qt::DirectConnection);
    QObject::connect(c, SIGNAL(fulcrum_moved(Fulcrum*,bool)), this, SLOT(fulcrumMoved(Fulcrum *, bool)));
    QObject::connect(c, SIGNAL(fulcrum_moved_handle(Fulcrum*,bool,int)), this, SLOT(fulcrumHandleMoved(Fulcrum *, bool, int)));
    QObject::connect(c, SIGNAL(fulcrum_type_changed(Fulcrum*,int)), this, SLOT(fulcrumTypeChanged(Fulcrum*,int)));

    invalidateSchema();
}

void DesignerScene::connectionDeleted(Connection*)
{
    invalidateSchema();
}

void DesignerScene::boxMoved(NodeBox *box)
{
    MovableGraphicsProxyWidget* proxy = widget_ctrl_->getProxy(box->getNode()->getUUID());
    proxy->setPos(box->pos());
    invalidateSchema();
}

void DesignerScene::fulcrumAdded(Fulcrum * f)
{
    FulcrumWidget* w = new FulcrumWidget(f, dispatcher_);
    addItem(w);
    fulcrum_2_widget_[f] = w;
    fulcrum_last_pos_[f] = f->pos();
    fulcrum_last_type_[f] = f->type();
    fulcrum_last_hin_[f] = f->handleIn();
    fulcrum_last_hout_[f] = f->handleOut();

    clearSelection();
    w->setSelected(true);
    setFocusItem(w, Qt::MouseFocusReason);

    invalidateSchema();
}

void DesignerScene::fulcrumDeleted(Fulcrum* f)
{
    std::map<Fulcrum*, FulcrumWidget*>::iterator pos = fulcrum_2_widget_.find(f);
    assert(pos != fulcrum_2_widget_.end());

    delete pos->second;
    fulcrum_2_widget_.erase(pos);

    invalidateSchema();
}

void DesignerScene::fulcrumMoved(Fulcrum * f, bool dropped)
{
    if(dropped) {
        dispatcher_->execute(Command::Ptr(new command::MoveFulcrum(f->connectionId(), f->id(), fulcrum_last_pos_[f], f->pos())));
        fulcrum_last_pos_[f] = f->pos();
    }
    invalidateSchema();
}

void DesignerScene::fulcrumHandleMoved(Fulcrum * f, bool dropped, int /*which*/)
{
    if(dropped) {
        dispatcher_->execute(Command::Ptr(new command::ModifyFulcrum(f->connectionId(), f->id(),
                                                                     fulcrum_last_type_[f], fulcrum_last_hin_[f], fulcrum_last_hout_[f],
                                                                     f->type(), f->handleIn(), f->handleOut())));
        fulcrum_last_type_[f] = f->type();
        fulcrum_last_hin_[f] = f->handleIn();
        fulcrum_last_hout_[f] = f->handleOut();

    }
    invalidateSchema();
}

void DesignerScene::fulcrumTypeChanged(Fulcrum */*f*/, int /*type*/)
{
    invalidateSchema();
}

void DesignerScene::addTemporaryConnection(Connectable *from, const QPointF& end)
{
    assert(from);

    TempConnection temp;
    temp.from = from;
    temp.to = end;

    temp_.push_back(temp);

    update();
}

void DesignerScene::addTemporaryConnection(Connectable *from, Connectable *to)
{
    assert(from);
    assert(to);

    TempConnection temp;
    temp.from = from;
    temp.to = widget_ctrl_->getPort(to)->centerPoint();

    temp_.push_back(temp);
}

void DesignerScene::deleteTemporaryConnections()
{
    temp_.clear();
}

void DesignerScene::deleteTemporaryConnectionsAndRepaint()
{
    deleteTemporaryConnections();
    update();
}

void DesignerScene::drawConnection(QPainter *painter, Connection& connection)
{
    if(!connection.from()->isOutput() || !connection.to()->isInput()) {
        return;
    }

    ConnectorOut* from = dynamic_cast<ConnectorOut*> (connection.from());
    ConnectorIn* to = dynamic_cast<ConnectorIn*> (connection.to());

    Port* fromp = widget_ctrl_->getPort(from);
    Port* top = widget_ctrl_->getPort(to);

    if(!fromp || !top) {
        return;
    }

    QPoint p1 = topLevelParentWidget(fromp)->pos() + fromp->centerPoint();
    QPoint p2 = topLevelParentWidget(top)->pos() + top->centerPoint();

    int id = connection.id();

    ccs = CurrentConnectionState();

    ccs.highlighted = (highlight_connection_id_ == id);
    ccs.error = (to->isError() || from->isError());
    ccs.disabled = (!from->isEnabled() || !to->isEnabled());
    ccs.async = from->isAsync() || to->isAsync();
    ccs.minimized_from = fromp->isMinimizedSize();
    ccs.minimized_to = top->isMinimizedSize();

    drawConnection(painter, p1, p2, id,
                   fromp->isFlipped() ? Fulcrum::IN : Fulcrum::OUT,
                   top->isFlipped() ? Fulcrum::OUT : Fulcrum::IN);

    int f = connection.activity();

    drawActivity(painter, f, from);
    drawActivity(painter, f, to);
}

void DesignerScene::drawConnection(QPainter *painter, const QPointF& from, const QPointF& real_to, int id, Fulcrum::Type from_type, Fulcrum::Type to_type)
{
    QPointF to = real_to;
    to.setX(to.x() - ARROW_LENGTH);

    painter->setRenderHint(QPainter::Antialiasing);

    ccs.minimized = ccs.minimized_from || ccs.minimized_to;
    ccs.r = ccs.minimized ? 2 : 4;

    double max_slack_height = 40.0;
    double mindist_for_slack = 60.0;
    double slack_smooth_distance = 300.0;

    QPointF diff = (to - from);

    double direct_length = hypot(diff.x(), diff.y());

    Connection::Ptr connection = graph_->getConnectionWithId(id);

    Fulcrum::Ptr current(new Fulcrum(connection.get(), from, from_type, from, from));
    Fulcrum::Ptr last = current;

    std::vector<Fulcrum::Ptr> targets;
    if(id >= 0) {
        targets = connection->getFulcrums();
    }
    targets.push_back(Fulcrum::Ptr(new Fulcrum(connection.get(), to, to_type, to, to)));

    int sub_section = 0;

    QPointF cp1, cp2;

    // paths
    typedef std::pair<QPainterPath, int> Path;
    std::vector<Path> paths;

    // generate lines
    Q_FOREACH(Fulcrum::Ptr fulcrum, targets) {
        QPoint offset;
        QPoint delta;
        if(direct_length > mindist_for_slack) {
            double offset_factor = std::min(1.0, (direct_length - mindist_for_slack) / slack_smooth_distance);

            delta = QPoint(std::max(offset_factor * mindist_for_slack, std::abs(0.45 * (diff).x())), 0);
            offset = QPoint(0, offset_factor * max_slack_height);
        }

        QPainterPath path(current->pos());

        if(current->type() == Fulcrum::OUT) {
            cp1 = current->pos() + delta + offset;

        } else if(current->type() == Fulcrum::IN) {
            cp1 = current->pos() - delta + offset;

        } else {
            const Fulcrum::Ptr& last = targets[sub_section-1];
            if(last->type() == Fulcrum::LINEAR) {
                cp1 = current->pos();
            } else {
                cp1 = current->pos() + current->handleOut();
            }
        }

        if(fulcrum->type() == Fulcrum::IN) {
            cp2 = fulcrum->pos() - delta + offset;

        } else if(fulcrum->type() == Fulcrum::OUT) {
            cp2 = fulcrum->pos() + delta + offset;
        } else {
            if(fulcrum->type() == Fulcrum::LINEAR) {
                cp2 = fulcrum->pos();
            } else {
                cp2 = fulcrum->pos() + fulcrum->handleIn();
            }
        }

        path.cubicTo(cp1, cp2, fulcrum->pos());

        paths.push_back(std::make_pair(path, sub_section));

        last = current;
        current = fulcrum;
        ++sub_section;
    }

    // arrow
    QPolygonF arrow;
    arrow.append(QPointF(to.x()+ ARROW_LENGTH, to.y()));
    arrow.append(QPointF(to.x(), to.y() -ARROW_LENGTH/2.0));
    arrow.append(QPointF(to.x(), to.y() + ARROW_LENGTH/2.0));
    arrow.append(QPointF(to.x()+ ARROW_LENGTH, to.y()));

    QPainterPath arrow_path;
    arrow_path.addPolygon(arrow);

    // reset brush if it is set
    painter->setBrush(QBrush());

    // draw
    if(ccs.highlighted) {
        painter->setPen(QPen(Qt::black, ccs.r + 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        foreach(const Path& path, paths) {
            painter->drawPath(path.first);
        }
        painter->drawPath(arrow_path);

        painter->setPen(QPen(Qt::white, ccs.r + 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        foreach(const Path& path, paths) {
            painter->drawPath(path.first);
        }
        painter->drawPath(arrow_path);
    }

    painter->setPen(makeLinePen(from, to));
    foreach(const Path& path, paths) {
        painter->drawPath(path.first);

        if(id >= 0 && schema_dirty_) {
            QPen schema_pen = QPen(QColor(id2rgb(id, path.second)), ccs.r * 1.75, Qt::SolidLine, Qt::RoundCap,Qt::RoundJoin);
            schematics_painter->setPen(schema_pen);
            schematics_painter->drawPath(path.first);
        }
    }
    painter->setBrush(input_color_);
    painter->setPen(QPen(painter->brush(), 1.0));
    painter->drawPath(arrow_path);
}

void DesignerScene::drawPort(QPainter *painter, NodeBox* box, Port *p)
{
    // reset brush if it is set
    painter->setBrush(QBrush());

    Connectable* c = p->getAdaptee();
    bool right = c->isOutput() ^ p->isFlipped();

    if(!p->isMinimizedSize()) {
        int font_size = 10;
        int lines = 3;

        QFont font;
        font.setPixelSize(font_size);
        painter->setFont(font);

        QString text = c->getLabel().c_str();

        if(text.length() != 0) {
            text += "\n";
        }
        text += c->getType()->name().c_str();

        QFontMetrics metrics(font);

        int dx = 160;
        int dy = lines * metrics.height();

        QPointF pos = box->pos() + p->centerPoint();
        QRectF rect(pos + QPointF(right ? 2*connector_radius_ : -2*connector_radius_-dx, -dy / 2.0), QSize(dx, dy));

        QTextOption opt(Qt::AlignVCenter | (right ? Qt::AlignLeft : Qt::AlignRight));
        QColor color = c->isOutput() ? output_color_ : input_color_;
        QPen p = painter->pen();
        p.setColor(color.dark());
        painter->setPen(p);
        painter->drawText(rect, text, opt);
    }
}

void DesignerScene::drawActivity(QPainter *painter, int life, Connectable* c)
{
    // reset brush if it is set
    painter->setBrush(QBrush());

    Port* port = widget_ctrl_->getPort(c);
    if(port && life > 0) {
        int r = std::min(Settings::activity_marker_max_lifetime_, life);
        double f = r / static_cast<double> (Settings::activity_marker_max_lifetime_);

        int min = port->width() / 2 - 2;
        int max = min * 1.2;
        double w = min + f * (max - min);

        QColor color = c->isOutput() ? output_color_ : input_color_;
        color.setAlpha(activity_marker_min_opacity_ + (activity_marker_max_opacity_ - activity_marker_min_opacity_) * f);

        QPointF pos = topLevelParentWidget(port)->pos() + port->centerPoint();

        painter->setPen(QPen(color, w));
        painter->drawEllipse(pos, w, w);
    }
}

bool DesignerScene::showConnectionContextMenu()
{
    QMenu menu;
    QAction* reset = new QAction("reset connection", &menu);
    menu.addAction(reset);
    QAction* del = new QAction("delete connection", &menu);
    menu.addAction(del);

    QAction* selectedItem = menu.exec(QCursor::pos());

    if(selectedItem == del) {
        dispatcher_->execute(graph_->deleteConnectionById(highlight_connection_id_));

    } else if(selectedItem == reset) {
        dispatcher_->execute(graph_->deleteAllConnectionFulcrumsCommand(highlight_connection_id_));
    }

    return true;
}

void DesignerScene::invalidateSchema()
{
    schema_dirty_ = true;
    update();
}

void DesignerScene::refresh()
{
    invalidateSchema();
}
