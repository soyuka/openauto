#include <memory>
#include <boost/asio.hpp>
#include <QVideoSink>
#include <QVideoWidget>
#include <QWidget>
#include <QtTest>
#include "openauto/Configuration/Configuration.hpp"
#include "openauto/Service/ServiceFactory.hpp"

using openauto::configuration::Configuration;
using openauto::service::ServiceFactory;

class ServiceFactoryTest: public QObject
{
    Q_OBJECT

private slots:
    void rendersIntoAnInternalVideoWidgetByDefault();
    void rendersIntoAHostVideoSink();

private:
    boost::asio::io_service ioService_;
};

void ServiceFactoryTest::rendersIntoAnInternalVideoWidgetByDefault()
{
    QWidget activeArea;
    ServiceFactory factory(ioService_, std::make_shared<Configuration>(), &activeArea);

    const auto widgets = activeArea.findChildren<QVideoWidget*>();
    QCOMPARE(widgets.size(), 1);
    QCOMPARE(factory.videoOutput()->videoSink(), widgets.front()->videoSink());
}

void ServiceFactoryTest::rendersIntoAHostVideoSink()
{
    QWidget activeArea;
    bool active = false;
    ServiceFactory factory(ioService_, std::make_shared<Configuration>(), &activeArea, [&active](bool isActive) { active = isActive; });
    QVideoSink sink;

    factory.setVideoSink(&sink);

    QVERIFY(activeArea.findChildren<QVideoWidget*>().isEmpty());
    QCOMPARE(factory.videoOutput()->videoSink(), &sink);

    emit factory.videoOutput()->startPlayback();
    QTRY_VERIFY(active);
    emit factory.videoOutput()->stopPlayback();
    QTRY_VERIFY(!active);
}

QTEST_MAIN(ServiceFactoryTest)
#include "ServiceFactoryTest.moc"
