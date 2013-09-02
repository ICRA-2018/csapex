#include "ctrl_main_menu.h"
#include <QFileDialog>
#include <iostream>
#include <fstream>
#include "ctrl_map_view.h"
#include "ctrl_cmpcore_bridge.h"

CtrlMainWindow::CtrlMainWindow(TerraTrainerWindow *main_window) :
    main_window_(main_window),
    zoom_(100.0)
{
}

void CtrlMainWindow::setupUi(Ui::TerraTrainerWindow *ui)
{

}

void CtrlMainWindow::loadImage()
{
    QString filename = QFileDialog::getOpenFileName(
                0,
                tr("Open Image"),
                QDir::currentPath(),
                tr("jpg (*.jpg);; png (*.png);; pgm (.pgm);;All files (*.*)") );

    if(!filename.isNull()) {
        image_path_ = filename;

        CMPCoreBridge::Ptr bridge = boost::shared_dynamic_cast<CMPCoreBridge>(main_window_->controllers_[Controller::Bridge]);

        if(bridge != NULL)
            Q_EMIT loadIMAGE(image_path_);
    }
}

void CtrlMainWindow::loadClassifier()
{
    QString filename = QFileDialog::getOpenFileName(
                0,
                tr("Choose file..."),
                QDir::currentPath(),
                tr("(*.yaml);;All files (*.*)") );

    if(!filename.isNull()) {
        Controller::Ptr class_ed = main_window_->controllers_[Controller::Class];
        Controller::Ptr bridge   = main_window_->controllers_[Controller::Bridge];
        Controller::Ptr pref     = main_window_->controllers_[Controller::Preferences];

        if(bridge != NULL || class_ed != NULL || pref != NULL) {
            std::ifstream in(filename.toUtf8().constData());
            if(!in.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            YAML::Parser p(in);
            YAML::Node document;
            p.GetNextDocument(document);
            class_ed->read(document);
            pref->read(document);
            bridge->read(document);
            in.close();
        }

    }
}

void CtrlMainWindow::loadSettings()
{
    QString filename = QFileDialog::getOpenFileName(
                0,
                tr("Choose file..."),
                QDir::currentPath(),
                tr("(*.yaml);;All files (*.*)") );

    if(!filename.isNull()) {
        Controller::Ptr class_ed = main_window_->controllers_[Controller::Class];
        Controller::Ptr pref     = main_window_->controllers_[Controller::Preferences];

        if(class_ed != NULL || pref != NULL) {
            std::ifstream in(filename.toUtf8().constData());
            if(!in.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            YAML::Parser p(in);
            YAML::Node document;
            p.GetNextDocument(document);
            class_ed->read(document);
            pref->read(document);
            in.close();
        }

    }
}

void CtrlMainWindow::loadROIs()
{
    QString filename = QFileDialog::getOpenFileName(
                0,
                tr("Choose file..."),
                QDir::currentPath(),
                tr("(*.yaml);;All files (*.*)") );

    if(!filename.isNull()) {
        Controller::Ptr view        = main_window_->controllers_[Controller::MapView];
        Controller::Ptr class_ed    = main_window_->controllers_[Controller::Class];

        if(view != NULL || class_ed != NULL) {
            std::ifstream in(filename.toUtf8().constData());
            if(!in.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            YAML::Parser p(in);
            YAML::Node document;
            p.GetNextDocument(document);
            class_ed->read(document);
            view->read(document);
            in.close();
        }

    }
}

void CtrlMainWindow::saveSettings()
{
    QString filename = QFileDialog::getSaveFileName(
                0,
                tr("Choose destination..."),
                QDir::currentPath(),
                tr("(*.yaml)") );
    if( !filename.isNull() )
    {
        Controller::Ptr class_ed = main_window_->controllers_[Controller::Class];
        Controller::Ptr pref = main_window_->controllers_[Controller::Preferences];

        if(pref != NULL || class_ed != NULL) {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;
            class_ed->write(emitter);
            pref->write(emitter);
            emitter << YAML::EndMap;

            QRegExp rx (".+(\\.yaml$)");
            if(!rx.exactMatch(filename))
                filename += ".yaml";

            std::ofstream out(filename.toUtf8().constData());
            if(!out.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            out << emitter.c_str();
            out.close();
        }
    }

}

void CtrlMainWindow::saveClassifierProject()
{
    QString filename = QFileDialog::getSaveFileName(
                0,
                tr("Choose destination..."),
                QDir::currentPath(),
                tr("(*.yaml)") );
    if( !filename.isNull() )
    {
        Controller::Ptr class_ed = main_window_->controllers_[Controller::Class];
        Controller::Ptr bridge = main_window_->controllers_[Controller::Bridge];

        if(bridge != NULL || class_ed != NULL) {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;
            class_ed->write(emitter);
            bridge->write(emitter);
            emitter << YAML::EndMap;

            QRegExp rx (".+(\\.yaml$)");
            if(!rx.exactMatch(filename))
                filename += ".yaml";

            std::ofstream out(filename.toUtf8().constData());
            if(!out.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            out << emitter.c_str();
            out.close();
        }
    }

}

void CtrlMainWindow::saveClassifierRaw()
{
    QString filename = QFileDialog::getSaveFileName(
                0,
                tr("Choose destination..."),
                QDir::currentPath(),
                tr("(*.yaml)") );
    if( !filename.isNull() )
    {
        Controller::Ptr bridge = main_window_->controllers_[Controller::Bridge];

        if(bridge != NULL) {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;
            bridge->write(emitter);
            emitter << YAML::EndMap;

            QRegExp rx (".+(\\.yaml$)");
            if(!rx.exactMatch(filename))
                filename += ".yaml";

            std::ofstream out(filename.toUtf8().constData());
            if(!out.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            out << emitter.c_str();
            out.close();
        }
    }
}

void CtrlMainWindow::saveCrops()
{
    QString dir = QFileDialog::getExistingDirectory(
                0,
                tr("Choose directory"),
                QDir::currentPath());
    if(!dir.isNull()) {
        CtrlMapView::Ptr map_view = boost::dynamic_pointer_cast<CtrlMapView>(main_window_->controllers_[Controller::MapView]);

        if(map_view != NULL)
            map_view->saveSelectedCrops(dir);
    }
}

void CtrlMainWindow::saveROIs()
{
    QString filename = QFileDialog::getSaveFileName(
                0,
                tr("Choose destination..."),
                QDir::currentPath(),
                tr("(*.yaml)") );
    if( !filename.isNull() )
    {
        Controller::Ptr map_view = main_window_->controllers_[Controller::MapView];
        Controller::Ptr class_ed = main_window_->controllers_[Controller::Class];

        if(map_view != NULL && class_ed != NULL) {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "IMAGE" << YAML::Value << image_path_.toUtf8().constData();
            class_ed->write(emitter);
            map_view->write(emitter);
            emitter << YAML::EndMap;

            QRegExp rx (".+(\\.yaml$)");
            if(!rx.exactMatch(filename))
                filename += ".yaml";

            std::ofstream out(filename.toUtf8().constData());
            if(!out.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            out << emitter.c_str();
            out.close();
        }
    }

}

void CtrlMainWindow::saveBatchTemplate()
{
    QString filename = QFileDialog::getSaveFileName(
                0,
                tr("Choose destination..."),
                QDir::currentPath(),
                tr("(*.yaml)") );
    if( !filename.isNull() )
    {
        Controller::Ptr class_ed = main_window_->controllers_[Controller::Class];
        CMPCoreBridge::Ptr bridge  = boost::dynamic_pointer_cast<CMPCoreBridge>(main_window_->controllers_[Controller::Bridge]);

        if(bridge != NULL && class_ed != NULL) {
            Q_EMIT syncSettingsCore();
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;
            class_ed->write(emitter);
            bridge->writeCore(emitter);
            emitter << YAML::Key << "WORK_PATH" << YAML::Value << "";
            emitter << YAML::Key << "ROI_FILES" << YAML::Value << YAML::BeginSeq;
            emitter << "path to roi file ...";
            emitter << YAML::EndSeq;
            emitter << YAML::EndMap;

            QRegExp rx (".+(\\.yaml$)");
            if(!rx.exactMatch(filename))
                filename += ".yaml";

            std::ofstream out(filename.toUtf8().constData());
            if(!out.is_open()) {
                std::cerr << "File couldn't be openened!" << std::endl;
                return;
            }

            out << emitter.c_str();
            out.close();
        }
    }
}

void CtrlMainWindow::zoomIn()
{
    zoom_ += 12.5;
    snapZoom();
    Q_EMIT zoom(zoom_);
}

void CtrlMainWindow::zoomOut()
{
    zoom_ -= 12.5;
    snapZoom();
    Q_EMIT zoom(zoom_);
}

void CtrlMainWindow::zoomReset()
{
    zoom_ = 100.0;
    Q_EMIT zoom(zoom_);
}

void CtrlMainWindow::zoomUpdate(double factor)
{
    zoom_ = factor;
}
