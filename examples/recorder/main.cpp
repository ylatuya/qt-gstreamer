/*
    Copyright (C) 2011 Collabora Ltd.
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ui_recorder.h"

#include <QtCore/QDir>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMessageBox>
#else
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>
#endif

#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/ElementFactory>
#include <QGst/ChildProxy>
#include <QGst/PropertyProbe>
#include <QGst/Pipeline>
#include <QGst/Pad>
#include <QGst/Event>
#include <QGst/Message>
#include <QGst/Bus>

#ifdef Q_WS_X11
# include <QtGui/QX11Info>
#endif


class Recorder : public QDialog
{
    Q_OBJECT
public:
    Recorder(QWidget *parent = 0);

private:
    enum Device { AudioSrc, VideoSrc };
    void findDevices(Device device);
    void probeForDevices(const QGst::PropertyProbePtr & propertyProbe);

    QGst::BinPtr createAudioSrcBin();
    QGst::BinPtr createVideoSrcBin();

    void start();
    void stop();

    void onBusMessage(const QGst::MessagePtr & message);

private Q_SLOTS:
    void on_startStopButton_clicked();

private:
    Ui::Recorder m_ui;
    QGst::PropertyProbePtr m_audioProbe;
    QGst::PropertyProbePtr m_videoProbe;
    QGst::PipelinePtr m_pipeline;
};

Recorder::Recorder(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);

    //setup the device combo boxes
    findDevices(AudioSrc);
    findDevices(VideoSrc);

    QGst::ElementFactoryPtr ximagesrc = QGst::ElementFactory::find("ximagesrc");
    if (!ximagesrc) {
        //if we don't have ximagesrc, disable the choice to use it
        m_ui.videoSourceComboBox->removeItem(1);
    } else {
#ifdef Q_WS_X11
        //setup the default screen to be the screen that this dialog is shown
        m_ui.displayNumSpinBox->setValue(QX11Info::appScreen());
#endif
    }

    //set default output file
    m_ui.outputFileEdit->setText(QDir::currentPath() + QDir::separator() + "out.ogv");
}

void Recorder::findDevices(Device device)
{
    const char *srcElementName = (device == AudioSrc) ? "autoaudiosrc" : "autovideosrc";
    QGst::ElementPtr src = QGst::ElementFactory::make(srcElementName);

    if (!src) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to create element \"%1\". Make sure you have "
                                 "gstreamer-plugins-good installed").arg(srcElementName));
        return;
    }

    QGst::PropertyProbePtr propertyProbe;

    //autoaudiosrc and autovideosrc implement the child proxy interface
    //and create the correct child element when they go to the READY state
    src->setState(QGst::StateReady);
    QGst::ChildProxyPtr childProxy = src.dynamicCast<QGst::ChildProxy>();
    if (childProxy && childProxy->childrenCount() > 0) {
        //the actual source is the first child
        //this source usually implements the property probe interface
        propertyProbe = childProxy->childByIndex(0).dynamicCast<QGst::PropertyProbe>();
    }
    //we got a reference to the underlying propertyProbe, so we don't need src anymore.
    src->setState(QGst::StateNull);

    //Most sources and sinks have a "device" property which supports probe
    //and probing it returns all the available devices on the system.
    //Here we try to make use of that to list the system's devices
    //and if it fails, we just leave the source to use its default device.
    if (propertyProbe && propertyProbe->propertySupportsProbe("device")) {
        ((device == AudioSrc) ? m_audioProbe : m_videoProbe) = propertyProbe;
        probeForDevices(propertyProbe);

        //this signal will notify us when devices change
        QGlib::connect(propertyProbe, "probe-needed",
                       this, &Recorder::probeForDevices, QGlib::PassSender);
    } else {
        QComboBox *box = (device == AudioSrc) ? m_ui.audioDeviceComboBox : m_ui.videoDeviceComboBox;
        box->addItem(tr("Default"));
    }
}

void Recorder::probeForDevices(const QGst::PropertyProbePtr & propertyProbe)
{
    QComboBox *box = (propertyProbe == m_audioProbe) ?
                     m_ui.audioDeviceComboBox : m_ui.videoDeviceComboBox;

    box->clear();
    box->addItem(tr("Default"));

    //get a list of devices that the element supports
    QList<QGlib::Value> devices = propertyProbe->probeAndGetValues("device");

    Q_FOREACH(const QGlib::Value & device, devices) {
        //set the element's device to the current device and retrieve its
        //human-readable name through the "device-name" property
        propertyProbe->setProperty("device", device);
        QString deviceName = propertyProbe->property("device-name").toString();

        //add the device on the combobox
        box->addItem(QString("%1 (%2)").arg(deviceName, device.toString()),
                     device.toString());
    }
}

QGst::BinPtr Recorder::createAudioSrcBin()
{
    QGst::BinPtr audioBin;

    try {
        audioBin = QGst::Bin::fromDescription("autoaudiosrc name=\"audiosrc\" ! audioconvert ! "
                                              "audioresample ! audiorate ! speexenc ! queue");
    } catch (const QGlib::Error & error) {
        qCritical() << "Failed to create audio source bin:" << error;
        return QGst::BinPtr();
    }

    //set the source's properties
    QVariant device = m_ui.audioDeviceComboBox->itemData(m_ui.audioDeviceComboBox->currentIndex());
    if (device.isValid()) {
        QGst::ElementPtr src = audioBin->getElementByName("audiosrc");

        //autoaudiosrc creates the actual source in the READY state
        src->setState(QGst::StateReady);

        QGst::ChildProxyPtr childProxy = src.dynamicCast<QGst::ChildProxy>();
        if (childProxy && childProxy->childrenCount() > 0) {
            //the actual source is the first child
            QGst::ObjectPtr realSrc = childProxy->childByIndex(0);
            realSrc->setProperty("device", device.toString());
        }
    }

    return audioBin;
}

QGst::BinPtr Recorder::createVideoSrcBin()
{
    QGst::BinPtr videoBin;

    try {
        if (m_ui.videoSourceComboBox->currentIndex() == 0) { //camera
            videoBin = QGst::Bin::fromDescription("autovideosrc name=\"videosrc\" ! "
                                                  "ffmpegcolorspace ! theoraenc ! queue");
        } else { //screencast
            videoBin = QGst::Bin::fromDescription("ximagesrc name=\"videosrc\" ! "
                                                  "ffmpegcolorspace ! theoraenc ! queue");
        }
    } catch (const QGlib::Error & error) {
        qCritical() << "Failed to create video source bin:" << error;
        return QGst::BinPtr();
    }

    //set the source's properties
    if (m_ui.videoSourceComboBox->currentIndex() == 0) { //camera
        QVariant device = m_ui.videoDeviceComboBox->itemData(m_ui.videoDeviceComboBox->currentIndex());
        if (device.isValid()) {
            QGst::ElementPtr src = videoBin->getElementByName("videosrc");

            //autovideosrc creates the actual source in the READY state
            src->setState(QGst::StateReady);

            QGst::ChildProxyPtr childProxy = src.dynamicCast<QGst::ChildProxy>();
            if (childProxy && childProxy->childrenCount() > 0) {
                //the actual source is the first child
                QGst::ObjectPtr realSrc = childProxy->childByIndex(0);
                realSrc->setProperty("device", device.toString());
            }
        }
    } else { //screencast
        videoBin->getElementByName("videosrc")->setProperty("screen-num", m_ui.displayNumSpinBox->value());
    }

    return videoBin;
}

void Recorder::start()
{
    QGst::BinPtr audioSrcBin = createAudioSrcBin();
    QGst::BinPtr videoSrcBin = createVideoSrcBin();
    QGst::ElementPtr mux = QGst::ElementFactory::make("oggmux");
    QGst::ElementPtr sink = QGst::ElementFactory::make("filesink");

    if (!audioSrcBin || !videoSrcBin || !mux || !sink) {
        QMessageBox::critical(this, tr("Error"), tr("One or more elements could not be created. "
                              "Verify that you have all the necessary element plugins installed."));
        return;
    }

    sink->setProperty("location", m_ui.outputFileEdit->text());

    m_pipeline = QGst::Pipeline::create();
    m_pipeline->add(audioSrcBin, videoSrcBin, mux, sink);

    //link elements
    QGst::PadPtr audioPad = mux->getRequestPad("sink_%d");
    audioSrcBin->getStaticPad("src")->link(audioPad);

    QGst::PadPtr videoPad = mux->getRequestPad("sink_%d");
    videoSrcBin->getStaticPad("src")->link(videoPad);

    mux->link(sink);

    //connect the bus
    m_pipeline->bus()->addSignalWatch();
    QGlib::connect(m_pipeline->bus(), "message", this, &Recorder::onBusMessage);

    //go!
    m_pipeline->setState(QGst::StatePlaying);
    m_ui.startStopButton->setText(tr("Stop recording"));
}

void Recorder::stop()
{
    //stop recording
    m_pipeline->setState(QGst::StateNull);

    //clear the pointer, destroying the pipeline as its reference count drops to zero.
    m_pipeline.clear();

    //restore the button's text
    m_ui.startStopButton->setText(tr("Start recording"));
}

void Recorder::onBusMessage(const QGst::MessagePtr & message)
{
    switch (message->type()) {
    case QGst::MessageEos:
        //got end-of-stream - stop the pipeline
        stop();
        break;
    case QGst::MessageError:
        //check if the pipeline exists before destroying it,
        //as we might get multiple error messages
        if (m_pipeline) {
            stop();
        }
        QMessageBox::critical(this, tr("Pipeline Error"),
                              message.staticCast<QGst::ErrorMessage>()->error().message());
        break;
    default:
        break;
    }
}

void Recorder::on_startStopButton_clicked()
{
    if (m_pipeline) { //pipeline exists - destroy it
        //send an end-of-stream event to flush metadata and cause an EosMessage to be delivered
        m_pipeline->sendEvent(QGst::EosEvent::create());
    } else { //pipeline doesn't exist - start a new one
        start();
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QGst::init(&argc, &argv);

    Recorder r;
    r.show();

    return app.exec();
}

#include "main.moc"
