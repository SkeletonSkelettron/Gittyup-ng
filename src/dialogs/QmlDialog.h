//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef QMLDIALOG_H
#define QMLDIALOG_H

#include <QDialog>
#include <QVariantMap>

class QQuickItem;
class QQuickWidget;

// A dialog drawn by qrc:/qml/<name>.qml, which reads the properties of the
// subclass as 'dialog'. Subclasses call setContent() at the end of their
// constructor, once their properties are ready.
class QmlDialog : public QDialog {
  Q_OBJECT

public:
  QmlDialog(QWidget *parent = nullptr);
  ~QmlDialog() override;

protected:
  void showEvent(QShowEvent *event) override;

  void setContent(const QString &name,
                  const QVariantMap &context = QVariantMap());

  QQuickWidget *view() const { return mView; }
  QQuickItem *rootItem() const;

private:
  void updateSize();
  void syncView();

  QQuickWidget *mView = nullptr;
};

#endif
