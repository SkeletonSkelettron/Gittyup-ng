//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "QmlDialog.h"
#include "ui/qml/QmlSupport.h"
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

QmlDialog::QmlDialog(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
}

QmlDialog::~QmlDialog() {
  // The QML view references this object, so it has to go first.
  delete mView;
}

void QmlDialog::setContent(const QString &name, const QVariantMap &context) {
  QVariantMap map = context;
  map.insert("dialog", QVariant::fromValue<QObject *>(this));
  mView = QmlSupport::createView(name, map, this);
  layout()->addWidget(mView);

  // Follow the size of the content.
  if (QQuickItem *root = rootItem()) {
    connect(root, &QQuickItem::implicitWidthChanged, this,
            &QmlDialog::updateSize);
    connect(root, &QQuickItem::implicitHeightChanged, this,
            &QmlDialog::updateSize);
  }

  updateSize();
  mView->setFocus();
}

void QmlDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  updateSize();

  // Showing the dialog moves the focus to the first item, so move it to
  // the item that should have it.
  QTimer::singleShot(0, this, [this] {
    QQuickItem *root = rootItem();
    if (root && root->metaObject()->indexOfMethod("focusInitialItem()") >= 0)
      QMetaObject::invokeMethod(root, "focusInitialItem");
  });
}

void QmlDialog::syncView() {
  if (!mView)
    return;

  // The layout doesn't always follow a resize of the dialog that happens
  // while it's being shown.
  if (mView->geometry() != rect())
    mView->setGeometry(rect());

  // A QQuickWidget that is resized before its window is exposed keeps
  // drawing at its old size, so resize it again after that.
  if (mView->quickWindow()->size() == mView->size())
    return;

  QSize size = mView->size();
  mView->resize(size + QSize(0, 1));
  mView->resize(size);
}

QQuickItem *QmlDialog::rootItem() const {
  return mView ? mView->rootObject() : nullptr;
}

void QmlDialog::updateSize() {
  QQuickItem *root = rootItem();
  if (!root || !isVisible())
    return;

  QSize size(qCeil(root->implicitWidth()), qCeil(root->implicitHeight()));
  setMinimumSize(size);
  resize(size.expandedTo(QSize(width(), 0)));

  QTimer::singleShot(0, this, &QmlDialog::syncView);
  QTimer::singleShot(100, this, &QmlDialog::syncView);
}
