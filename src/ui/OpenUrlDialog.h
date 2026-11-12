// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>
#include <QStringList>

class QComboBox;
class QLabel;
class QPushButton;

namespace outplayer::ui {

class OpenUrlDialog : public QDialog {
    Q_OBJECT

public:
    explicit OpenUrlDialog(QWidget* parent = nullptr);
    ~OpenUrlDialog() override = default;

    [[nodiscard]] QString url() const;
    void setUrl(const QString& url);

    [[nodiscard]] QStringList recentUrls() const;
    void setRecentUrls(const QStringList& urls);

public slots:
    void accept() override;

private:
    void setupUi();
    void applyStyleSheet();
    void loadSettings();
    void saveSettings();

    QComboBox* m_urlCombo{nullptr};
    QLabel* m_errorLabel{nullptr};
    QPushButton* m_openButton{nullptr};
    QPushButton* m_cancelButton{nullptr};
    QStringList m_recentUrls;
};

} // namespace outplayer::ui

namespace outplayer {
using ui::OpenUrlDialog;
}
