#include "kwinrulemanager.h"

#include <KConfig>
#include <KConfigGroup>
#include <QDBusInterface>
#include <QDir>
#include <QMap>
#include <QStandardPaths>
#include <QUuid>

namespace {

const QLatin1String kGeneral("General");
const QLatin1String kRulesKey("rules");
const QLatin1String kOrderKey("Order");
const QLatin1String kLegacyCountKey("count");

QString defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (dir.isEmpty())
        return {};
    QDir().mkpath(dir);
    return dir + QStringLiteral("/kwinrulesrc");
}

// KWin rule "match" and "force" enums (see kwin/src/rules.h).
constexpr int kMatchExact = 1;
constexpr int kSetRuleForce = 2;
constexpr int kSetRuleRemember = 4;
constexpr int kTypeNormalWindow = 1; // NET::NormalMask

QMap<QString, QString> desiredEntries(const KWinRuleManager::RuleSpec &spec)
{
    QMap<QString, QString> e;
    e.insert(QStringLiteral("Description"), spec.description);
    e.insert(QStringLiteral("wmclass"), spec.wmClass);
    e.insert(QStringLiteral("wmclassmatch"), QString::number(kMatchExact));
    e.insert(QStringLiteral("wmclasscomplete"), QStringLiteral("false"));
    if (!spec.title.isEmpty()) {
        e.insert(QStringLiteral("title"), spec.title);
        e.insert(QStringLiteral("titlematch"), QString::number(kMatchExact));
    }
    e.insert(QStringLiteral("types"), QString::number(kTypeNormalWindow));
    e.insert(QStringLiteral("above"), spec.keepAbove ? QStringLiteral("true") : QStringLiteral("false"));
    e.insert(QStringLiteral("aboverule"), QString::number(kSetRuleForce));
    e.insert(QStringLiteral("skiptaskbar"), spec.skipTaskbar ? QStringLiteral("true") : QStringLiteral("false"));
    e.insert(QStringLiteral("skiptaskbarrule"), QString::number(kSetRuleForce));
    e.insert(QStringLiteral("skippager"), spec.skipTaskbar ? QStringLiteral("true") : QStringLiteral("false"));
    e.insert(QStringLiteral("skippagerrule"), QString::number(kSetRuleForce));
    if (spec.rememberPosition)
        e.insert(QStringLiteral("positionrule"), QString::number(kSetRuleRemember));
    return e;
}

} // namespace

KWinRuleManager::KWinRuleManager(const QString &configPath)
    : m_path(configPath.isEmpty() ? defaultPath() : configPath)
{
}

QString KWinRuleManager::findGroupByDescription(const QString &description) const
{
    if (m_path.isEmpty())
        return {};
    KConfig cfg(m_path, KConfig::SimpleConfig);
    const QStringList groups = cfg.groupList();
    for (const QString &name : groups) {
        if (name == kGeneral)
            continue;
        if (KConfigGroup(&cfg, name).readEntry("Description", QString()) == description)
            return name;
    }
    return {};
}

bool KWinRuleManager::ensureRule(const RuleSpec &spec)
{
    if (m_path.isEmpty() || spec.description.isEmpty() || spec.wmClass.isEmpty())
        return false;

    KConfig cfg(m_path, KConfig::SimpleConfig);
    KConfigGroup general(&cfg, kGeneral);
    QStringList rules = general.readEntry(kRulesKey, QStringList());
    const bool hasOrder = general.hasKey(kOrderKey);
    QStringList order = hasOrder ? general.readEntry(kOrderKey, QStringList()) : QStringList();

    QString groupName;
    const QStringList groups = cfg.groupList();
    for (const QString &name : groups) {
        if (name == kGeneral)
            continue;
        if (KConfigGroup(&cfg, name).readEntry("Description", QString()) == spec.description) {
            groupName = name;
            break;
        }
    }
    if (groupName.isEmpty())
        groupName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    bool changed = false;
    KConfigGroup rule(&cfg, groupName);
    const QMap<QString, QString> wanted = desiredEntries(spec);
    for (auto it = wanted.constBegin(); it != wanted.constEnd(); ++it) {
        if (rule.readEntry(it.key(), QString()) != it.value()) {
            rule.writeEntry(it.key(), it.value());
            changed = true;
        }
    }
    if (!spec.rememberPosition && rule.hasKey("positionrule")) {
        rule.deleteEntry("positionrule");
        changed = true;
    }
    if (spec.title.isEmpty() && rule.hasKey("title")) {
        rule.deleteEntry("title");
        rule.deleteEntry("titlematch");
        changed = true;
    }

    if (!rules.contains(groupName)) {
        rules.append(groupName);
        general.writeEntry(kRulesKey, rules);
        changed = true;
    }
    if (hasOrder && !order.contains(groupName)) {
        order.append(groupName);
        general.writeEntry(kOrderKey, order);
        changed = true;
    }
    // The legacy count= key makes KWin take its migration path; with an
    // explicit rules= list it is meaningless and only invites confusion.
    if (general.hasKey(kLegacyCountKey)) {
        general.deleteEntry(kLegacyCountKey);
        changed = true;
    }

    if (changed)
        cfg.sync();
    return changed;
}

bool KWinRuleManager::cleanupLegacy(const QStringList &descriptions)
{
    if (m_path.isEmpty())
        return false;

    KConfig cfg(m_path, KConfig::SimpleConfig);
    KConfigGroup general(&cfg, kGeneral);
    const QStringList rules = general.readEntry(kRulesKey, QStringList());

    bool changed = false;
    const QStringList groups = cfg.groupList();
    for (const QString &name : groups) {
        if (name == kGeneral || rules.contains(name))
            continue;
        bool numeric = false;
        name.toInt(&numeric);
        if (!numeric)
            continue;
        const QString desc = KConfigGroup(&cfg, name).readEntry("Description", QString());
        if (descriptions.contains(desc)) {
            cfg.deleteGroup(name);
            changed = true;
        }
    }
    if (general.hasKey(kLegacyCountKey)) {
        general.deleteEntry(kLegacyCountKey);
        changed = true;
    }
    if (changed)
        cfg.sync();
    return changed;
}

void KWinRuleManager::reloadKWin()
{
    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"));
    if (kwin.isValid())
        kwin.call(QStringLiteral("reconfigure"));
}
