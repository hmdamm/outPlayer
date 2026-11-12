// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "ui/OsdOverlay.h"

using namespace outplayer::ui;

class TestOsdOverlay : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testShowMessage();
    void testClear();
    void testPositions();
    void testMessageReplacement();
    void testFadeAndClearLifecycle();
    void testParentGeometryTracking();
    void testPaintEventExecution();
};

void TestOsdOverlay::initTestCase() {
    // Ensure GUI application is initialized
    QVERIFY(qApp != nullptr);
}

void TestOsdOverlay::testInitialState() {
    OsdOverlay osd;
    QCOMPARE(osd.currentText(), QString());
    QCOMPARE(osd.isVisibleOsd(), false);
    QCOMPARE(osd.opacity(), 0.0);
    QCOMPARE(osd.position(), OsdPosition::TopLeft);

    QVERIFY(osd.testAttribute(Qt::WA_TransparentForMouseEvents));
    QVERIFY(osd.testAttribute(Qt::WA_NoSystemBackground));
    QVERIFY(osd.testAttribute(Qt::WA_TranslucentBackground));
}

void TestOsdOverlay::testShowMessage() {
    OsdOverlay osd;
    osd.showMessage(QStringLiteral("Volume: 85%"), 1500, OsdPosition::TopLeft);

    QCOMPARE(osd.currentText(), QStringLiteral("Volume: 85%"));
    QCOMPARE(osd.isVisibleOsd(), true);
    QCOMPARE(osd.opacity(), 1.0);
    QCOMPARE(osd.position(), OsdPosition::TopLeft);
}

void TestOsdOverlay::testClear() {
    OsdOverlay osd;
    osd.showMessage(QStringLiteral("Seek: +00:00:05"), 1500, OsdPosition::Center);
    QCOMPARE(osd.isVisibleOsd(), true);

    osd.clear();
    QCOMPARE(osd.isVisibleOsd(), false);
    QCOMPARE(osd.currentText(), QString());
    QCOMPARE(osd.opacity(), 0.0);
}

void TestOsdOverlay::testPositions() {
    OsdOverlay osd;

    osd.showMessage(QStringLiteral("TopLeft"), 1500, OsdPosition::TopLeft);
    QCOMPARE(osd.position(), OsdPosition::TopLeft);

    osd.showMessage(QStringLiteral("Center"), 1500, OsdPosition::Center);
    QCOMPARE(osd.position(), OsdPosition::Center);

    osd.showMessage(QStringLiteral("BottomLeft"), 1500, OsdPosition::BottomLeft);
    QCOMPARE(osd.position(), OsdPosition::BottomLeft);
}

void TestOsdOverlay::testMessageReplacement() {
    OsdOverlay osd;
    osd.showMessage(QStringLiteral("Initial Message"), 2000, OsdPosition::TopLeft);
    QCOMPARE(osd.currentText(), QStringLiteral("Initial Message"));

    // Replace immediately with a different message
    osd.showMessage(QStringLiteral("Replaced Message"), 2000, OsdPosition::Center);
    QCOMPARE(osd.currentText(), QStringLiteral("Replaced Message"));
    QCOMPARE(osd.position(), OsdPosition::Center);
    QCOMPARE(osd.opacity(), 1.0);
    QCOMPARE(osd.isVisibleOsd(), true);
}

void TestOsdOverlay::testFadeAndClearLifecycle() {
    OsdOverlay osd;
    // Short duration for fast and deterministic test: 120ms total duration
    osd.showMessage(QStringLiteral("Fast fade"), 120);
    QCOMPARE(osd.isVisibleOsd(), true);
    QCOMPARE(osd.opacity(), 1.0);

    // Wait until timeout and fade completion
    QTRY_VERIFY_WITH_TIMEOUT(!osd.isVisibleOsd(), 600);
    QCOMPARE(osd.currentText(), QString());
    QCOMPARE(osd.opacity(), 0.0);
}

void TestOsdOverlay::testParentGeometryTracking() {
    QWidget parent;
    parent.resize(640, 480);
    parent.show();

    OsdOverlay* osd = new OsdOverlay(&parent);
    QCOMPARE(osd->size(), parent.size());

    // Trigger resize on parent
    parent.resize(800, 600);
    QCOMPARE(osd->size(), QSize(800, 600));
}

void TestOsdOverlay::testPaintEventExecution() {
    QWidget parent;
    parent.resize(640, 480);
    OsdOverlay osd(&parent);
    osd.resize(640, 480);

    // Test multi-line text painting at all 3 positions
    osd.showMessage(QStringLiteral("Header\nSecond Line"), 1000, OsdPosition::TopLeft);
    osd.repaint();

    osd.showMessage(QStringLiteral("Center Notification"), 1000, OsdPosition::Center);
    osd.repaint();

    osd.showMessage(QStringLiteral("Bottom Left Status"), 1000, OsdPosition::BottomLeft);
    osd.repaint();

    QVERIFY(osd.isVisibleOsd());
}

QTEST_MAIN(TestOsdOverlay)
#include "test_osd_overlay.moc"
