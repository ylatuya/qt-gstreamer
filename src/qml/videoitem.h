/*
    Copyright (C) 2012 Collabora Ltd. <info@collabora.com>
      @author George Kiagiadakis <george.kiagiadakis@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include "../QGst/Ui/graphicsvideosurface.h"
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtQuick/QQuickItem>
#else
#include <QtDeclarative/QDeclarativeItem>
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
class VideoItem : public QQuickItem
#else
class VideoItem : public QDeclarativeItem
#endif
{
    Q_OBJECT
    Q_PROPERTY(QGst::Ui::GraphicsVideoSurface* surface READ surface WRITE setSurface)
public:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    explicit VideoItem(QQuickItem *parent = 0);
#else
    explicit VideoItem(QDeclarativeItem *parent = 0);
#endif
    virtual ~VideoItem();

    QGst::Ui::GraphicsVideoSurface *surface() const;
    void setSurface(QGst::Ui::GraphicsVideoSurface *surface);

protected:
    virtual void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry);

private:
    QGst::Ui::GraphicsVideoWidget *m_widget;
};

#endif // VIDEOITEM_H
