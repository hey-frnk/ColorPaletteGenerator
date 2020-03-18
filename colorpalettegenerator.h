#ifndef COLORPALETTEGENERATOR_H
#define COLORPALETTEGENERATOR_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class ColorPaletteGenerator; }
QT_END_NAMESPACE

class ColorPaletteGenerator : public QMainWindow
{
    Q_OBJECT

public:
    ColorPaletteGenerator(QWidget *parent = nullptr);
    ~ColorPaletteGenerator();

private slots:
    void on_compute_clicked();

    void on_load_clicked();

    void on_s_1_toggled(bool checked);

    void on_s_2_toggled(bool checked);

    void on_s_3_toggled(bool checked);

    void on_save_clicked();

private:
    Ui::ColorPaletteGenerator *ui;
    QImage *src_large;
};
#endif // COLORPALETTEGENERATOR_H
