#pragma once

#include <QString>
#include <QStringList>

/**
 * Manages the KWin window rules that numi-kde needs on Wayland
 * (keep-above, skip taskbar/pager, "remember position").
 *
 * The rules live in ~/.config/kwinrulesrc, a KConfig file owned by KWin.
 * KWin 6 keeps the ordered list of rule groups in [General] rules=...
 * (newer builds add [General] Order=...), and names each group with a UUID.
 * KWin writes the remembered position back into the same group, so this
 * class only ever touches its own keys and never rewrites the file as a whole.
 */
class KWinRuleManager
{
public:
    struct RuleSpec {
        QString description;   ///< unique marker used to find the rule again
        QString wmClass;       ///< exact app_id / WM_CLASS match
        QString title;         ///< exact window title match; empty = any title
        bool keepAbove = true;
        bool skipTaskbar = true;
        bool rememberPosition = true;
    };

    /// @param configPath explicit file path (tests); empty = ~/.config/kwinrulesrc
    explicit KWinRuleManager(const QString &configPath = QString());

    /// Creates or updates the rule. Returns true when the file was modified.
    bool ensureRule(const RuleSpec &spec);

    /// Removes rule groups with any of the given descriptions that KWin does not
    /// list (legacy numeric groups written by numi-kde <= 0.1.80) and the
    /// legacy [General] count= key. Returns true when the file was modified.
    bool cleanupLegacy(const QStringList &descriptions);

    /// Group name of the rule carrying @p description, or empty.
    QString findGroupByDescription(const QString &description) const;

    /// Asks the running KWin to reload its configuration.
    static void reloadKWin();

    QString path() const { return m_path; }

private:
    QString m_path;
};
