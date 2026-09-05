#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QTimer>
#include "lineresult.h"

class EngineClient;

class DocumentModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY linesChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY linesChanged)
    Q_PROPERTY(bool hasTotal READ hasTotal NOTIFY linesChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int decimalPlaces READ decimalPlaces WRITE setDecimalPlaces NOTIFY decimalPlacesChanged)
    Q_PROPERTY(QString defaultCurrency READ defaultCurrency WRITE setDefaultCurrency NOTIFY defaultCurrencyChanged)
    Q_PROPERTY(double total READ total NOTIFY linesChanged)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)
    Q_PROPERTY(int networkStatus READ networkStatus NOTIFY networkStatusChanged)
    Q_PROPERTY(bool isWayland READ isWayland CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(int engineRestarts READ engineRestarts NOTIFY engineRestartsChanged)

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
    QString defaultCurrency() const;
    void setDefaultCurrency(const QString &currency);
    double total() const;
    bool autostart() const;
    void setAutostart(bool enable);
    int networkStatus() const;
    int engineRestarts() const;
    bool isWayland() const;
    QString version() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void copyResult(int row);
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void setKeepAbove(bool above);
    Q_INVOKABLE void prepareShow();
    Q_INVOKABLE void migrateLegacyState();
    Q_INVOKABLE void saveSession();
    Q_INVOKABLE void restoreSession(int index);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE QString completeWord(const QString &prefix);
    Q_INVOKABLE QStringList getCompletions(const QString &lineContext);
    Q_INVOKABLE QString highlightExample(const QString &line) const;

signals:
    void sourceChanged();
    void linesChanged();
    void historyChanged();
    void decimalPlacesChanged();
    void defaultCurrencyChanged();
    void autostartChanged();
    void networkStatusChanged();
    void engineRestartsChanged();

private:
    void evaluate();
    void onEvaluated(quint64 generation, const QList<LineResult> &lines);
    bool applyKWinRules(bool keepAbove);
    static QString autostartPath();
    static QString legacyAutostartPath();

    QString m_source;
    QList<LineResult> m_lines;
    int m_errorCount = 0;
    int m_resultCount = 0;
    bool m_hasTotal = false;
    EngineClient *m_engine;
    QVariantList m_history;
    int m_decimalPlaces = 3;
    QString m_defaultCurrency = QStringLiteral("USD");
    double m_total = 0.0;
    bool m_keepAbove = true;
    QTimer *m_debounceTimer;
    quint64 m_evalGeneration = 0;
};
