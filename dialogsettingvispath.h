#ifndef DIALOGSETTINGVISPATH_H
#define DIALOGSETTINGVISPATH_H

#include <QDialog>
#include <QFileDialog>
#include "ui_dialogsettingvispath.h"

namespace Ui {
class DialogSettingVisPath;
}

class DialogSettingVisPath : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettingVisPath(QWidget *parent = nullptr);

    void setPathToFile(const QString&);

    QString getPathToFile();

    void setText(const QString& );
    ~DialogSettingVisPath();

private slots:
    void on_okButton_clicked();

    void on_cancelButton_clicked();


    void on_toolButton_clicked();

private:
    Ui::DialogSettingVisPath *ui;
    QString pathToFile;
};

#endif // DIALOGSETTINGVISPATH_H
