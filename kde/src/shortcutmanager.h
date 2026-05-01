#pragma once

#include <QObject>
#include <QString>

class QAction;

class ShortcutManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sequence READ sequence WRITE setSequence NOTIFY sequenceChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit ShortcutManager(QAction *action, QObject *parent = nullptr);

    QString sequence() const;
    void setSequence(const QString &sequence);
    QString status() const;

signals:
    void sequenceChanged();
    void statusChanged();

private:
    bool registerShortcut(const QString &sequence, bool promptForConflicts);
    void setStatus(const QString &status);
    void updateActionText();

    QAction *m_action = nullptr;
    QString m_sequence;
    QString m_status;
};
