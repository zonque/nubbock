/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Wayland module
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QMouseEvent>
#include <QPainter>
#include <QMatrix4x4>
#include <QOpenGLFunctions>

#include "window.h"
#include "compositor.h"

#include <QtWaylandCompositor/qwaylandseat.h>

Window::Window()
    : m_compositor(0)
{
}

void Window::setCompositor(Compositor *comp) {
    m_compositor = comp;
}

void Window::initializeGL()
{
    QString backgroundImagePath =  QString::fromLocal8Bit(qgetenv("NUBBOCK_BACKGROUND_IMAGE"));

    if (!backgroundImagePath.isEmpty()) {
        QImage backgroundImage = QImage(backgroundImagePath);
        backgroundImage.invertPixels();
        m_backgroundTexture = new QOpenGLTexture(backgroundImage, QOpenGLTexture::DontGenerateMipMaps);
        m_backgroundTexture->setMinificationFilter(QOpenGLTexture::Nearest);
        m_backgroundImageSize = backgroundImage.size();
    }

    m_textureBlitter.create();
}

void Window::paintGL()
{
    m_compositor->startRender();

    QOpenGLFunctions *functions = context()->functions();
    functions->glClearColor(.0f, .165f, .31f, 0.5f);
    functions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_textureBlitter.bind();

    if (m_backgroundTexture) {
        QMatrix4x4 targetTransform = QOpenGLTextureBlitter::targetTransform(QRect(QPoint(0, 0), m_backgroundImageSize),
                                                                            QRect(QPoint(0, 0), size()));
        m_textureBlitter.blit(m_backgroundTexture->textureId(),
                              targetTransform,
                              QOpenGLTextureBlitter::OriginTopLeft);
    }

    functions->glEnable(GL_BLEND);
    functions->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Q_FOREACH (View *view, m_compositor->views()) {
        if (view->isCursor())
            continue;
        auto texture = view->getTexture();
        if (!texture)
            continue;
        GLuint textureId = texture->textureId();
        QWaylandSurface *surface = view->surface();
        if (surface && surface->hasContent()) {
            QSize s = surface->size();
            QPointF pos(0, 0);
            QRectF surfaceGeometry(pos, s);
            QOpenGLTextureBlitter::Origin surfaceOrigin =
                    view->currentBuffer().origin() == QWaylandSurface::OriginTopLeft
                    ? QOpenGLTextureBlitter::OriginTopLeft
                    : QOpenGLTextureBlitter::OriginBottomLeft;
            QMatrix4x4 targetTransform = QOpenGLTextureBlitter::targetTransform(surfaceGeometry, QRect(QPoint(), size()));
            m_textureBlitter.blit(textureId, targetTransform, surfaceOrigin);
        }
    }
    m_textureBlitter.release();
    m_compositor->endRender();
}

void Window::sendMouseEvent(QMouseEvent *e, View *target)
{
    QPointF mappedPos = e->localPos();
    if (target)
        mappedPos -= target->position();
    QMouseEvent viewEvent(e->type(), mappedPos, e->localPos(), e->button(), e->buttons(), e->modifiers());
    m_compositor->handleMouseEvent(target, &viewEvent);
}

void Window::mousePressEvent(QMouseEvent *e)
{
    if (m_mouseView.isNull())
        m_mouseView = viewAt(e->localPos());

    sendMouseEvent(e, m_mouseView);
}

void Window::mouseReleaseEvent(QMouseEvent *e)
{
    sendMouseEvent(e, m_mouseView);
}

void Window::keyPressEvent(QKeyEvent *e)
{
    m_compositor->defaultSeat()->sendKeyPressEvent(e->nativeScanCode());
}

void Window::keyReleaseEvent(QKeyEvent *e)
{
    m_compositor->defaultSeat()->sendKeyReleaseEvent(e->nativeScanCode());
}


