#include "shortcutmanager.h"

#include <QAction>
#include <QDebug>
#include <QKeySequence>
#include <QSettings>
#include <QWidget>

#ifdef NUMI_KDE_HAVE_GLOBAL_ACCEL
#include <KGlobalAccel>
#endif

ShortcutManager::ShortcutManager(QAction *action, QObject *parent)
    : QObject(parent)
    , m_action(action)
{
#ifdef NUMI_KDE_HAVE_GLOBAL_ACCEL
    // KGlobalAccel automatically loads the shortcut from kglobalshortcutsrc
    // based on the application name and objectName.
    KGlobalAccel::self()->setGlobalShortcut(m_action, QKeySequence(QStringLiteral("Ctrl+Alt+1")));
    
    QList<QKeySequence> shortcuts = KGlobalAccel::self()->shortcut(m_action);
    if (!shortcuts.isEmpty()) {
        m_sequence = shortcuts.first().toString(QKeySequence::PortableText);
    } else {
        m_sequence = QStringLiteral("Ctrl+Alt+1");
    }
#endif
    updateActionText();
}

QString ShortcutManager::sequence() const
{
    return m_sequence;
}

QString ShortcutManager::status() const
{
    return m_status;
}

void ShortcutManager::setSequence(const QString &sequence)
{
    const QString normalized = QKeySequence::fromString(sequence, QKeySequence::PortableText)
                                   .toString(QKeySequence::PortableText);
    if (normalized.isEmpty() || normalized == m_sequence)
        return;

    if (!registerShortcut(normalized, true)) {
        qWarning() << "Failed to register global shortcut" << normalized;
        return;
    }

    m_sequence = normalized;
    setStatus(QStringLiteral("Shortcut saved"));
    updateActionText();
    emit sequenceChanged();
}

bool ShortcutManager::registerShortcut(const QString &sequence, bool promptForConflicts)
{
    if (!m_action)
        return false;

    const QKeySequence shortcut = QKeySequence::fromString(sequence, QKeySequence::PortableText);
    if (shortcut.isEmpty())
        return false;

#ifdef NUMI_KDE_HAVE_GLOBAL_ACCEL
    auto *globalAccel = KGlobalAccel::self();
    if (promptForConflicts && !KGlobalAccel::isGlobalShortcutAvailable(shortcut)) {
        const auto conflicts = KGlobalAccel::globalShortcutsByKey(shortcut);
        if (!KGlobalAccel::promptStealShortcutSystemwide(nullptr, conflicts, shortcut)) {
            setStatus(QStringLiteral("Shortcut is already used"));
            return false;
        }
        KGlobalAccel::stealShortcutSystemwide(shortcut);
    }

    // This commits the new shortcut to kglobalshortcutsrc
    const bool ok = globalAccel->setShortcut(m_action, {shortcut});
    setStatus(ok ? QString() : QStringLiteral("Shortcut is not available"));
    return ok;
#else
    setStatus(QStringLiteral("Global shortcuts require KDE GlobalAccel"));
    return true;
#endif
}

void ShortcutManager::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void ShortcutManager::updateActionText()
{
    if (m_action)
        m_action->setText(QStringLiteral("Show / Hide  (%1)").arg(m_sequence));
}
