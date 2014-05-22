#ifndef CONNECTION_H
#define CONNECTION_H

/// COMPONENT
#include <csapex/view/selectable.h>
#include <csapex/csapex_fwd.h>

/// SYSTEM
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <vector>
#include <QPoint>

namespace csapex
{

class Connection : public QObject, public Selectable {

    Q_OBJECT

    friend class command::AddFulcrum;
    friend class command::MoveFulcrum;
    friend class command::DeleteFulcrum;
    friend class GraphIO;
    friend class Graph;
    friend class ConnectionSelectionModel;
    friend class BoxSelectionModel;

public:
    typedef boost::shared_ptr<Connection> Ptr;

    struct Fulcrum {
        enum Type {
            CURVE = 0,
            LINEAR = 1,
            HANDLE = LINEAR,
            OUT = 10,
            IN = 11
        };
        Fulcrum(const QPointF& p, int t)
            : type(t), pos(p)
        {}

        int type;
        QPointF pos;
    };

public:
    friend std::ostream& operator << (std::ostream& out, const Connection& c) {
        out << "Connection: [" << c.from() << " / " << c.to() << "]";
        return out;
    }

public:
    Connection(ConnectorOut* from, ConnectorIn* to);
    Connection(ConnectorOut* from, ConnectorIn* to, int id);

    Connectable* from() const;
    Connectable* to() const;
    int id() const;

    bool contains(Connectable* c) const;

    int activity() const;
    void tick();

private Q_SLOTS:
    void messageSentEvent();

Q_SIGNALS:
    void fulcrum_added(Connection*);
    void fulcrum_moved(Connection*);
    void fulcrum_deleted(Connection*);

public:
    bool operator == (const Connection& c) const;

    std::vector<Fulcrum> getFulcrums() const;
    int getFulcrumCount() const;
    Fulcrum getFulcrum(int fulcrum_id);

protected:
    Connection(Connectable* from, Connectable* to);

private:
    /// COMMANDS
    void addFulcrum(int subsection, const QPointF& pos, int type);
    void moveFulcrum(int fulcrum_id, const QPointF &pos);
    void deleteFulcrum(int fulcrum_id);

protected:
    Connectable* from_;
    Connectable* to_;

    std::vector<Fulcrum> fulcrums_;

    int id_;

    double message_count;

    static int next_connection_id_;
};

}

#endif // CONNECTION_H
