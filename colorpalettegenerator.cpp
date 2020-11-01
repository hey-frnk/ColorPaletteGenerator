#include "colorpalettegenerator.h"
#include "ui_colorpalettegenerator.h"

#include <QFileDialog>
#include "opencv2/opencv.hpp"
#include "asmOpenCV.h"

ColorPaletteGenerator::ColorPaletteGenerator(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ColorPaletteGenerator)
{
    ui->setupUi(this);
    this->setWindowTitle("Pinterestable Color Palette Generator by The VFD Collective ♥");
    srand(time(NULL));

    src_large = nullptr;
}

ColorPaletteGenerator::~ColorPaletteGenerator()
{
    delete src_large;
    delete ui;
}

QString img_path = ":/img/res0.jpg";
QString img_folder = QDir::homePath();

void ColorPaletteGenerator::on_compute_clicked()
{
    // Read in image
    if(src_large) delete src_large;
    src_large = new QImage(img_path);
    // Scale image window
    int new_height = src_large->height() / (float)src_large->width() * ui->original->width();
    int new_height_difference = ui->original->height() - new_height;
    ui->original->resize(ui->original->width(), new_height);
    ui->clustered->resize(ui->original->width(), new_height);
    ui->panel->move(ui->panel->pos().x(), ui->panel->pos().y() - new_height_difference);
    this->resize(this->width(), this->height() - new_height_difference);

    QImage src_q = src_large->scaled(ui->original->width(), ui->original->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->original->setPixmap(QPixmap::fromImage(src_q));

    // Parse Image
    // Article: https://medium.com/@andrisgauracs/generating-color-palettes-from-movies-with-python-16503077c025
    // Parse: https://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap/
    cv::Mat source_img = ASM::QImageToCvMat(src_q.rgbSwapped());

    // Serialize, float
    int serialized_size = source_img.total();
    cv::Mat serialized_data = source_img.reshape(1, serialized_size);
    serialized_data.convertTo(serialized_data, CV_32F);

    // Perform k-Means
    int k = ui->k_slider->value(); // Random number between 8 and 11
    std::vector<int> labels;
    cv::Mat3f centers;
    // cv::kmeans(serialized_data, k, labels, cv::TermCriteria(), 1, cv::KMEANS_PP_CENTERS, centers);
    cv::kmeans(serialized_data, k, labels, cv::TermCriteria(CV_TERMCRIT_ITER, 10, 1.0), 3, cv::KMEANS_PP_CENTERS, centers);

    // Make a poster! (Clustering)
    for (int i = 0; i < (int)labels.size(); ++i) {
        serialized_data.at<float>(i, 0) = centers.at<float>(labels[i], 0);
        serialized_data.at<float>(i, 1) = centers.at<float>(labels[i], 1);
        serialized_data.at<float>(i, 2) = centers.at<float>(labels[i], 2);
    }

    // Un-Serialize, un-float
    cv::Mat destination = serialized_data.reshape(3, source_img.rows);
    destination.convertTo(source_img, CV_8UC3);
    cv::cvtColor(source_img, source_img, CV_BGR2RGB);

    // Display temporary result
    ui->clustered->setPixmap(ASM::cvMatToQPixmap(source_img));

    // Keep the most unique colors by iteratively removing the most similar centers
    bool remove_darkest_color = ui->skip_darkest->isChecked();
    bool remove_lightest_color = ui->skip_lightest->isChecked();
    struct c_dist { cv::Vec3f c; double dist; size_t a, b; };
    for(int i = 0; i < k - 6 - remove_darkest_color; ++i) {
        // Point distance
        std::vector<c_dist> u;
        for(size_t a = 0; a < centers.total() - 1; ++a)
            for(size_t b = a + 1; b < centers.total(); ++b)
                u.push_back({.dist = cv::norm(centers.at<cv::Vec3f>(a) - centers.at<cv::Vec3f>(b), cv::NORM_L2), .a = a, .b = b});
        std::sort(u.begin(), u.end(), [](const c_dist &a, const c_dist &b) -> bool { return a.dist > b.dist; });
        // Eliminate similar row
        cv::Mat3f b, c;
        centers(cv::Range(0, i % 2 ? u.back().a : u.back().b), cv::Range(0, centers.cols)).copyTo(b);
        centers(cv::Range((i % 2 ? u.back().a : u.back().b) + 1, centers.rows), cv::Range(0, centers.cols)).copyTo(c);
        if(!b.rows) centers = c.clone();
        else if(!c.rows) centers = b.clone();
        else {
            cv::vconcat(b, c, b);
            centers = b.clone();
        }
    }

    // Make it pretty by sorting it by energy
    std::sort(centers.begin(), centers.end(),
        [](const cv::Vec3f &a, const cv::Vec3f &b) -> bool
        { return a[0] + a[1] + a[2] > b[0] + b[1] + b[2]; }
    );

    // Paint!
    QLabel *palette[6][2] = {
        {ui->b_1, ui->h_1}, {ui->b_2, ui->h_2}, {ui->b_3, ui->h_3},
        {ui->b_4, ui->h_4}, {ui->b_5, ui->h_5}, {ui->b_6, ui->h_6}
    };
    int p = remove_lightest_color; // Padding
    for(int i = 0; i < 6; ++i) {
        // Set back color of label. Lazy ass way to show the colors
        QColor clr((uint8_t)centers.at<float>(i + p, 0), (uint8_t)centers.at<float>(i + p, 1), (uint8_t)centers.at<float>(i + p, 2));
        palette[i][0]->setStyleSheet(QString("background-color:" + clr.name() + ";"));
        palette[i][1]->setText(clr.name().toUpper());
    }
}


void ColorPaletteGenerator::on_load_clicked()
{
    img_path = QFileDialog::getOpenFileName(this, tr("Open Image"), img_folder, tr("Image Files (*.png *.jpg *.bmp)"));
    img_folder = QFileInfo(img_path).path();
}

void ColorPaletteGenerator::on_s_1_toggled(bool checked)
{
    if(checked) img_path = ":/img/res0.jpg";
}

void ColorPaletteGenerator::on_s_2_toggled(bool checked)
{
    if(checked) img_path = ":/img/res1.jpg";
}

void ColorPaletteGenerator::on_s_3_toggled(bool checked)
{
    if(checked) img_path = ":/img/res2.jpg";
}


/*// Create histogram
int *count_labels = new int[k]{0};
for(int i : labels) if(labels.at(i) == i) ++count_labels[i];
delete[] count_labels;*/

void ColorPaletteGenerator::on_save_clicked()
{
    int src_width = ui->original->width(), color_height = ui->b_1->height(), color_width = ui->b_1->width();
    int border = 20, margin = 20, color_spacing = 10;

    // Width = 10 (border left) + width + 10 (border right)
    int output_original_width = border + 2 * src_width;
    int output_image_width = border + output_original_width + border;
    // Height = 10 (border upper) + height + 20 (margin) + color_dim (color) + 10 (border lower)
    int output_original_height = src_large->height() / (float)src_large->width() * output_original_width;
    int output_image_height = border + output_original_height + margin + color_height + border;

    // Empty white
    cv::Mat output_image(output_image_height, output_image_width, CV_8UC3, cv::Scalar(255, 255, 255));
    // Fill with down/upscaled original image
    QImage _output_original_scaled = src_large->scaled(output_original_width, output_original_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    cv::Mat output_original_scaled = ASM::QImageToCvMat(_output_original_scaled);
    output_original_scaled.copyTo(output_image(cv::Rect(border, border, output_original_scaled.cols, output_original_scaled.rows)));

    // Fill in palette rectangles
    QColor palette[6] = {
        QColor(ui->h_1->text()), QColor(ui->h_2->text()), QColor(ui->h_3->text()),
        QColor(ui->h_4->text()), QColor(ui->h_5->text()), QColor(ui->h_6->text())
    };
    for (int i = 0; i < 6; ++i) {
        cv::Mat single_color(color_height, color_width, CV_8UC3, cv::Scalar(palette[i].blue(), palette[i].green(), palette[i].red()));
        single_color.copyTo(output_image(cv::Rect(border + i * (color_width + color_spacing), border + output_original_height + margin, color_width, color_height)));
    }

    // Write to disk
    QString save_dir = img_folder;
    QString output_image_path = QFileDialog::getSaveFileName(this, tr("Save Image"), save_dir + "/" + QFileInfo(img_path).fileName());
    QImage q_output_image(ASM::cvMatToQImage(output_image));
    q_output_image.save(output_image_path);
    // cv::imwrite(std::string(output_image_path.toLatin1().data()), output_image);
}

void ColorPaletteGenerator::on_export_flcolor_clicked()
{
    // Export to
    QString file_path = QFileDialog::getSaveFileName(this, "Save custom color", QDir::homePath(), "Fluorescence Custom Color File (*.flclr)");
    if(file_path.isEmpty()) return;

    QColor palette[6] = {
        QColor(ui->h_1->text()), QColor(ui->h_2->text()), QColor(ui->h_3->text()),
        QColor(ui->h_4->text()), QColor(ui->h_5->text()), QColor(ui->h_6->text())
    };

    QFile save_file(file_path);
    save_file.open(QIODevice::WriteOnly);
    QDataStream output_stream(&save_file);
    for(QColor c : palette) {
        c.setHslF(c.hueF(), std::pow(c.hslSaturationF(), 0.2), std::pow(c.lightnessF(), 1.5));
        output_stream << c;
    }
    for(volatile QColor c : palette) output_stream << (quint8)0; // QString("x%1").arg((uint8_t)c, 2, 16, QChar('0'));

    save_file.close();
}

void ColorPaletteGenerator::on_reorder_clicked()
{
    QColor palette[6] = {
        QColor(ui->h_1->text()), QColor(ui->h_2->text()), QColor(ui->h_3->text()),
        QColor(ui->h_4->text()), QColor(ui->h_5->text()), QColor(ui->h_6->text())
    };
    // Shuffle
    std::random_shuffle(palette, palette + 6);

    QLabel *re_palette[6][2] = {
        {ui->b_1, ui->h_1}, {ui->b_2, ui->h_2}, {ui->b_3, ui->h_3},
        {ui->b_4, ui->h_4}, {ui->b_5, ui->h_5}, {ui->b_6, ui->h_6}
    };
    for(int i = 0; i < 6; ++i) {
        // Set back color of label. Lazy ass way to show the colors
        re_palette[i][0]->setStyleSheet(QString("background-color:" + palette[i].name() + ";"));
        re_palette[i][1]->setText(palette[i].name().toUpper());
    }
}

void ColorPaletteGenerator::on_overlay_clicked()
{
    QLabel *re_palette[6][2] = {
        {ui->b_1, ui->h_1}, {ui->b_2, ui->h_2}, {ui->b_3, ui->h_3},
        {ui->b_4, ui->h_4}, {ui->b_5, ui->h_5}, {ui->b_6, ui->h_6}
    };

    if(!reference_color_stored) {
        // Backup reference
        QColor reference_color_tmp[6] = {
            QColor(ui->h_1->text()), QColor(ui->h_2->text()), QColor(ui->h_3->text()),
            QColor(ui->h_4->text()), QColor(ui->h_5->text()), QColor(ui->h_6->text())
        };
        for(int i = 0; i < 6; ++i) {
            // Set back color of label. Lazy ass way to show the colors
            reference_color[i] = reference_color_tmp[i];
            QColor new_color = reference_color_tmp[i];

            new_color.setHslF(
                        new_color.hueF(),
                        std::pow(new_color.hslSaturationF(), (2000.0f - ui->sat_boost->value()) / 1000.0f),
                        std::pow(new_color.lightnessF(), (2000.0f - ui->l_boost->value()) / 1000.0f)
                        );
            re_palette[i][0]->setStyleSheet(QString("background-color:" + new_color.name() + ";"));
            re_palette[i][1]->setText(new_color.name().toUpper());
        }
        reference_color_stored = true;
        ui->overlay->setText("Overlay Off");
    } else {
        // Restore reference
        for(int i = 0; i < 6; ++i) {
            // Set back color of label. Lazy ass way to show the colors
            re_palette[i][0]->setStyleSheet(QString("background-color:" + reference_color[i].name() + ";"));
            re_palette[i][1]->setText(reference_color[i].name().toUpper());
        }
        reference_color_stored = false;
        ui->overlay->setText("Overlay On");
    }
}

void ColorPaletteGenerator::on_sat_boost_valueChanged(int value)
{
    ui->sat_boost_val->setText("S-scaling: " + QString("%1").arg((float)value / 1000.0f));
}

void ColorPaletteGenerator::on_l_boost_valueChanged(int value)
{
    ui->l_boost_val->setText("L-scaling: " + QString("%1").arg((float)value / 1000.0f));
}

void ColorPaletteGenerator::on_k_slider_valueChanged(int value)
{
    ui->k_value->setText("K: " + QString("%1").arg(value));
}
