#include <QFileDialog>
#include <QFile>
#include "patcher.h"
#include "ui_patcher.h"
#include "about.h"

const char Patcher::signature[] = {0xD6, 0xE3, 0x69, 0xA0,
                                   0x53, 0x0E, 0xE5, 0x23,
                                   0x45, 0xB1, 0xA4, 0xCC,
                                   0xC6, 0x79, 0x8E, 0xEC};

Patcher::Patcher(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Patcher)
{
    ui->setupUi(this);
    memset(patch_offset, 0, sizeof(patch_offset));
    memset(patch_size, 0, sizeof(patch_size));
    ui->patch_list->addItem(PATCH_FILE);
    ui->patch_list->addItem(PATCH_RESTOREFILE);
    ui->patch_list->setCurrentIndex(0);
    connect(
        ui->msg_list->model(),
        SIGNAL(rowsInserted ( const QModelIndex &, int, int ) ),
        ui->msg_list,
        SLOT(scrollToBottom ())
    );
}

void Patcher::on_iso_search_clicked() {
    QString file = QFileDialog::getOpenFileName(this,
         tr("Abrir ISO"), NULL, tr("Imagen iso (*.iso)"));
    if(!file.isEmpty()) {
        ui->iso_path->insertPlainText(file);
        filename = file;
    }
}

void Patcher::fill_tables(QFile *patchfile) {
    patchfile->seek(0);
    patchfile->read(reinterpret_cast<char *>(&patch_count), 4);
    for(unsigned int i = 0; i < patch_count; i++) {
        patchfile->read(reinterpret_cast<char *>(&patch_offset[i]), 4);
        patchfile->read(reinterpret_cast<char *>(&patch_size[i]), 4);
    }
    data_start = ((patch_count + 1) * 8);
    if(data_start % 16 > 0)
        data_start += 16 - (data_start % 16);
}

void Patcher::write_file(QFile *iso, QString patchfile, int mode) {
    char buffer[1024];
    char message[64];
    QFile patch(patchfile);
    if(patch.open(QIODevice::ReadOnly)) {
        fill_tables(&patch);
        patch.seek(data_start);
        for(unsigned int i = 0;i < patch_count;i++) {
            quint32 patch_len = patch_size[i];
            iso->seek(DATABIN_OFFSET + patch_offset[i]);
            for(unsigned int j = 0; j < patch_len; j+= 1024) {
                patch.read(buffer, 1024);
                iso->write(buffer, 1024);
            }
            if(mode == 0)
                sprintf(message, "Aplicando parche (%i/%i)...OK", i+1, patch_count);
            else
                sprintf(message, "Quitando parche (%i/%i)...OK", i+1, patch_count);
            ui->msg_list->addItem(message);
        }
        patch.close();
        ui->msg_list->addItem("Parcheo completado");
    } else {
        ui->msg_list->addItem("Archivo de parcheo no encontrado");
    }
}

void Patcher::on_iso_patch_clicked() {
    ui->msg_list->clear();
    if(!filename.isEmpty()) {
        QFile file(filename);
        if(file.open(QIODevice::ReadWrite)) {
            file.seek(DATABIN_OFFSET);
            char buffer[16];
            if(file.read(buffer, sizeof(buffer)) == 16) {
                if(!memcmp(buffer, signature, 16)) {
                    ui->msg_list->addItem("data.bin encontrado, comenzando parcheo");
                    write_file(&file, ui->patch_list->currentText(), ui->patch_list->currentIndex());
                } else {
                    ui->msg_list->addItem("data.bin no encontrado dentro del ISO");
                }
            } else {
                ui->msg_list->addItem("No se pudo leer correctamente la ISO");
            }
        } else {
            ui->msg_list->addItem("No se pudo abrir la ISO");
        }
    } else {
        ui->msg_list->addItem("Debe seleccionar una ISO");
    }
}

void Patcher::on_actionAcerca_de_Qt_triggered() {
    QApplication::aboutQt();
}

void Patcher::on_actionBuscar_triggered() {
    on_iso_search_clicked();
}

void Patcher::on_actionParchear_triggered() {
    on_iso_patch_clicked();
}

void Patcher::on_actionAcerca_de_triggered() {
    About w;
    w.exec();
}

Patcher::~Patcher() {
    delete ui;
}

