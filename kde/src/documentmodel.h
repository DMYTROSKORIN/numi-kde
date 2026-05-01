#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QProcess>
#include <QVariantList>

class QalcBridge;

class DocumentModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY linesChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY linesChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)

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
    QVariantList history() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void copyResult(int row);
    Q_INVOKABLE void setKeepAbove(bool above);
    Q_INVOKABLE void saveSession();
    Q_INVOKABLE void restoreSession(int index);
    Q_INVOKABLE void clearHistory();

signals:
    void sourceChanged();
    void linesChanged();
    void historyChanged();

private:
    void evaluate();
    static QString firstDiagnostic(const QJsonObject &line);

    QString m_source;
    QJsonArray m_lines;
    int m_errorCount = 0;
    int m_resultCount = 0;
    QalcBridge *m_qalc;
    QVariantList m_history;
};
