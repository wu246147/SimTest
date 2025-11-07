#ifndef MYTEXTITEM_H
#define MYTEXTITEM_H

#include <QGraphicsTextItem>


class MyTextItem: public QGraphicsTextItem
{
public:
    explicit MyTextItem(const QString &text, QGraphicsItem *parent = nullptr)
        : QGraphicsTextItem(text, parent)
    {
        // 关键：启用几何变化通知
        setFlags(QGraphicsItem::ItemSendsGeometryChanges);
    }
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if(change == ItemPositionChange && !m_posInitialized)
        {
            m_posInitialized = true;
            return QPointF(m_customX, m_customY);  // 返回你想要的位置
        }
        return QGraphicsTextItem::itemChange(change, value);
    }

public:
    void setCustomPos(qreal x, qreal y)
    {
        m_customX = x;
        m_customY = y;
    }

private:
    bool m_posInitialized = false;
    qreal m_customX = 0;
    qreal m_customY = 0;
};
#endif // MYTEXTITEM_H
