#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantList>

class QalcBridge;

class DocumentModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY linesChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY linesChanged)
    Q_PROPERTY(bool hasTotal READ hasTotal NOTIFY linesChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int decimalPlaces READ decimalPlaces WRITE setDecimalPlaces NOTIFY decimalPlacesChanged)
    Q_PROPERTY(double total READ total NOTIFY linesChanged)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)

public:
    enum Roles {
        LineRole = Qt::UserRole + 1,
        ResultRole,
        OkRole,
        DiagnosticRole,
        HighlightedHtmlRole,
    };

    explicit DocumentModel(QObject *parent = nullptr);
    ~DocumentModel();

    QString source() const;
    void setSource(const QString &source);
    int errorCount() const;
    int resultCount() const;
    bool hasTotal() const;
    QVariantList history() const;
    int decimalPlaces() const;
    void setDecimalPlaces(int places);
    double total() const;
    bool autostart() const;
    void setAutostart(bool enable);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void copyResult(int row);
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void setKeepAbove(bool above);
    Q_INVOKABLE void saveSession();
    Q_INVOKABLE void restoreSession(int index);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE QString completeWord(const QString &prefix);
    Q_INVOKABLE QString highlightExample(const QString &line) const;

signals:
    void sourceChanged();
    void linesChanged();
    void historyChanged();
    void decimalPlacesChanged();
    void autostartChanged();

private:
    void evaluate();
    static QString firstDiagnostic(const QJsonObject &line);
    static void setKWinKeepAboveRule(bool enabled);
    static void reloadKWinRules();

    QString m_source;
    QJsonArray m_lines;
    int m_errorCount = 0;
    int m_resultCount = 0;
    bool m_hasTotal = false;
    QalcBridge *m_qalc;
    QVariantList m_history;
    int m_decimalPlaces = 3;
    double m_total = 0.0;
    bool m_kwinRuleApplied = false;
    bool m_keepAbove = true;
};
