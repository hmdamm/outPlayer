// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/OpenUrlDialog.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/SettingsManager.h"

namespace outplayer::ui {

namespace {
constexpr int kMaxRecentUrls = 20;
const QString kRecentUrlsKey = QStringLiteral("network/recent_urls");
} // namespace

OpenUrlDialog::OpenUrlDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Open URL / Network Stream"));
    setModal(true);
    resize(520, 160);

    setupUi();
    applyStyleSheet();
    loadSettings();
}

void OpenUrlDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    auto* promptLabel = new QLabel(tr("Enter streaming link or URL (HTTP, HTTPS, HLS, RTMP, RTSP):"), this);
    mainLayout->addWidget(promptLabel);

    m_urlCombo = new QComboBox(this);
    m_urlCombo->setEditable(true);
    m_urlCombo->setInsertPolicy(QComboBox::NoInsert);
    if (auto* lineEdit = m_urlCombo->lineEdit()) {
        lineEdit->setPlaceholderText(tr("https://example.com/live/stream.m3u8"));
        lineEdit->setClearButtonEnabled(true);
        connect(lineEdit, &QLineEdit::returnPressed, this, &OpenUrlDialog::accept);
    }
    mainLayout->addWidget(m_urlCombo);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #ff5555; font-size: 11px;"));
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    m_openButton = new QPushButton(tr("Open"), this);
    m_openButton->setDefault(true);
    m_openButton->setCursor(Qt::PointingHandCursor);
    connect(m_openButton, &QPushButton::clicked, this, &OpenUrlDialog::accept);
    buttonLayout->addWidget(m_openButton);

    mainLayout->addLayout(buttonLayout);
}

void OpenUrlDialog::applyStyleSheet()
{
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #1c1c1c;"
        "  color: #eeeeee;"
        "  font-family: 'Segoe UI', 'Ubuntu', sans-serif;"
        "  font-size: 12px;"
        "}"
        "QLabel {"
        "  color: #cccccc;"
        "}"
        "QComboBox {"
        "  background-color: #2b2b2b;"
        "  color: #ffffff;"
        "  border: 1px solid #3c3c3c;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "}"
        "QComboBox:focus, QComboBox:on {"
        "  border: 1px solid #ffaa00;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #222222;"
        "  color: #ffffff;"
        "  selection-background-color: #383838;"
        "  selection-color: #ffaa00;"
        "  border: 1px solid #3c3c3c;"
        "}"
        "QLineEdit {"
        "  background-color: transparent;"
        "  color: #ffffff;"
        "  border: none;"
        "}"
        "QPushButton {"
        "  background-color: #333333;"
        "  color: #dddddd;"
        "  border: 1px solid #444444;"
        "  border-radius: 4px;"
        "  padding: 6px 18px;"
        "  min-width: 70px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #444444;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #252525;"
        "}"
        "QPushButton[default=\"true\"] {"
        "  background-color: #ffaa00;"
        "  color: #111111;"
        "  font-weight: bold;"
        "  border: 1px solid #e69900;"
        "}"
        "QPushButton[default=\"true\"]:hover {"
        "  background-color: #ffbb22;"
        "  color: #000000;"
        "}"
        "QPushButton[default=\"true\"]:pressed {"
        "  background-color: #e69900;"
        "}"
    ));
}

QString OpenUrlDialog::url() const
{
    if (!m_urlCombo) {
        return {};
    }
    return m_urlCombo->currentText().trimmed();
}

void OpenUrlDialog::setUrl(const QString& url)
{
    if (m_urlCombo) {
        m_urlCombo->setEditText(url.trimmed());
    }
}

QStringList OpenUrlDialog::recentUrls() const
{
    return m_recentUrls;
}

void OpenUrlDialog::setRecentUrls(const QStringList& urls)
{
    m_recentUrls = urls;
    if (m_urlCombo) {
        const QString current = m_urlCombo->currentText();
        m_urlCombo->clear();
        m_urlCombo->addItems(m_recentUrls);
        m_urlCombo->setEditText(current);
    }
}

void OpenUrlDialog::loadSettings()
{
    const QVariant stored = core::SettingsManager::instance().get(kRecentUrlsKey);
    if (stored.isValid()) {
        m_recentUrls = stored.toStringList();
        if (m_urlCombo) {
            m_urlCombo->clear();
            m_urlCombo->addItems(m_recentUrls);
            m_urlCombo->setCurrentIndex(-1);
            m_urlCombo->setEditText(QString());
        }
    }
}

void OpenUrlDialog::saveSettings()
{
    core::SettingsManager::instance().set(kRecentUrlsKey, m_recentUrls);
}

void OpenUrlDialog::accept()
{
    const QString targetUrl = url();
    if (targetUrl.isEmpty()) {
        if (m_errorLabel) {
            m_errorLabel->setText(tr("Please enter a valid media or streaming URL."));
            m_errorLabel->setVisible(true);
        }
        return;
    }

    if (m_errorLabel) {
        m_errorLabel->setVisible(false);
    }

    // Add to recent URLs (prepending and deduplicating)
    m_recentUrls.removeAll(targetUrl);
    m_recentUrls.prepend(targetUrl);
    while (m_recentUrls.size() > kMaxRecentUrls) {
        m_recentUrls.removeLast();
    }

    saveSettings();
    QDialog::accept();
}

} // namespace outplayer::ui
