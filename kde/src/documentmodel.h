#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QProcess>

class QalcBridge;

class DocumentModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY linesChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY linesChanged)

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

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void copyResult(int row);

signals:
    void sourceChanged();
    void linesChanged();

private:
    void evaluate();
    static QString runHighlightWorker(const QString &source);
    static QString firstDiagnostic(const QJsonObject &line);

    QString m_source;
    QJsonArray m_lines;
    int m_errorCount = 0;
    int m_resultCount = 0;
    QalcBridge *m_qalc;
};
