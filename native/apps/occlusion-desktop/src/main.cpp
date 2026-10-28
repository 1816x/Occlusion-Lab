#include "occlusion/evaluation/mesh_session.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkMatrix4x4.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindowToImageFilter.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {
namespace core = occlusion::core;
namespace evaluation = occlusion::evaluation;
namespace mesh_io = occlusion::mesh_io;

vtkSmartPointer<vtkActor> mesh_actor(const mesh_io::TriangleMesh& mesh,
                                     const std::array<double, 3>& color) {
  auto points = vtkSmartPointer<vtkPoints>::New();
  for (const auto& point : mesh.vertices_meters)
    points->InsertNextPoint(point.x, point.y, point.z);
  auto faces = vtkSmartPointer<vtkCellArray>::New();
  for (const auto& triangle : mesh.triangles) {
    const vtkIdType ids[3] = {static_cast<vtkIdType>(triangle[0]),
                              static_cast<vtkIdType>(triangle[1]),
                              static_cast<vtkIdType>(triangle[2])};
    faces->InsertNextCell(3, ids);
  }
  auto polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->SetPolys(faces);
  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(polydata);
  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(color[0], color[1], color[2]);
  actor->GetProperty()->SetOpacity(0.68);
  return actor;
}

QString error_text(const mesh_io::ObjFileError& error) {
  return QString::fromStdString(error.filename.empty() ? error.field + ": " + error.message
                                                       : mesh_io::describe_obj_file_error(error));
}

QString classification_text(core::ContactClassification value) {
  switch (value) {
  case core::ContactClassification::separated:
    return "Separated";
  case core::ContactClassification::touching:
    return "Touching";
  case core::ContactClassification::penetrating:
    return "Penetrating";
  }
  return "Unknown";
}

class MainWindow final : public QWidget {
public:
  explicit MainWindow(QString initial_directory = {}) {
    setWindowTitle("Occlusion Lab — synthetic desktop viewer");
    resize(1120, 720);

    directory_ = new QLineEdit(std::move(initial_directory));
    fixed_name_ = new QLineEdit("fixed-box.obj");
    moving_name_ = new QLineEdit("moving-box.obj");
    auto* browse = new QPushButton("Browse…");
    import_ = new QPushButton("Import OBJ pair");
    opening_ = spin_box(0, 250);
    protrusion_ = spin_box(0, 50);
    lateral_ = spin_box(-50, 50);
    evaluate_ = new QPushButton("Evaluate pose");
    preset_ = new QComboBox;
    preset_->addItem("Closing", static_cast<int>(core::SweepPreset::closing));
    preset_->addItem("Protrusive", static_cast<int>(core::SweepPreset::protrusive));
    preset_->addItem("Left lateral", static_cast<int>(core::SweepPreset::left_lateral));
    preset_->addItem("Right lateral", static_cast<int>(core::SweepPreset::right_lateral));
    frames_ = new QSpinBox;
    frames_->setRange(2, 61);
    frames_->setValue(31);
    sweep_button_ = new QPushButton("Run sweep");
    frame_slider_ = new QSlider(Qt::Horizontal);
    frame_slider_->setEnabled(false);
    export_ = new QPushButton("Export JSON v1");
    export_->setEnabled(false);
    status_ = new QLabel("Import two synthetic, closed OBJ meshes to begin.");
    status_->setWordWrap(true);
    scene_ = new QLabel;
    scene_->setMinimumSize(640, 480);
    scene_->setAlignment(Qt::AlignCenter);
    scene_->setStyleSheet("background: #101820; color: white;");
    scene_->setText("VTK scene appears after import");

    auto* form = new QFormLayout;
    auto* directory_row = new QHBoxLayout;
    directory_row->addWidget(directory_);
    directory_row->addWidget(browse);
    form->addRow("Directory", directory_row);
    form->addRow("Fixed OBJ", fixed_name_);
    form->addRow("Moving OBJ", moving_name_);
    form->addRow(import_);
    form->addRow("Opening (mm)", opening_);
    form->addRow("Protrusion (mm)", protrusion_);
    form->addRow("Lateral (mm)", lateral_);
    form->addRow(evaluate_);
    form->addRow("Sweep", preset_);
    form->addRow("Frames", frames_);
    form->addRow(sweep_button_);
    form->addRow("Selected frame", frame_slider_);
    form->addRow(export_);
    auto* rotate_left = new QPushButton("Rotate left");
    auto* rotate_right = new QPushButton("Rotate right");
    auto* camera_row = new QHBoxLayout;
    camera_row->addWidget(rotate_left);
    camera_row->addWidget(rotate_right);
    form->addRow("Camera", camera_row);
    auto* legend = new QLabel(
        "Axes: X red, Y green, Z blue. Engine lengths: meters; displayed distances: mm.");
    legend->setWordWrap(true);
    form->addRow(legend);
    form->addRow(status_);
    auto* layout = new QHBoxLayout(this);
    layout->addLayout(form, 1);
    layout->addWidget(scene_, 2);

    renderer_ = vtkSmartPointer<vtkRenderer>::New();
    renderer_->SetBackground(0.06, 0.10, 0.15);
    window_ = vtkSmartPointer<vtkRenderWindow>::New();
    window_->SetOffScreenRendering(1);
    window_->SetSize(800, 600);
    window_->AddRenderer(renderer_);
    add_axes();
    set_busy(false);

    connect(browse, &QPushButton::clicked, this, [this] {
      const auto selected = QFileDialog::getExistingDirectory(
          this, "Select synthetic OBJ directory", directory_->text());
      if (!selected.isEmpty())
        directory_->setText(selected);
    });
    connect(import_, &QPushButton::clicked, this, [this] { import_pair(); });
    connect(evaluate_, &QPushButton::clicked, this, [this] { evaluate_pose(); });
    connect(sweep_button_, &QPushButton::clicked, this, [this] { evaluate_sweep(); });
    connect(frame_slider_, &QSlider::valueChanged, this, [this](int index) {
      if (sweep_ && index < static_cast<int>(sweep_->frames.size()))
        show_result(sweep_->frames[static_cast<std::size_t>(index)].evaluation);
    });
    connect(export_, &QPushButton::clicked, this, [this] { export_json(); });
    connect(rotate_left, &QPushButton::clicked, this, [this] { rotate_camera(-15); });
    connect(rotate_right, &QPushButton::clicked, this, [this] { rotate_camera(15); });
  }

  bool smoke(const std::filesystem::path& directory) {
    if (!load_pair(directory, "fixed-box.obj", "moving-box.obj"))
      return false;
    const auto pose =
        session_->evaluator.evaluate({core::Meters{0}, core::Meters{0}, core::Meters{0}});
    if (!pose || pose.value().classification != core::ContactClassification::separated)
      return false;
    pose_ = pose.value();
    show_result(*pose_);
    const auto sweep = session_->evaluator.evaluate_sweep(core::SweepPreset::closing, 11);
    if (!sweep || sweep.value().frames.size() != 11)
      return false;
    sweep_ = sweep.value();
    show_result(sweep_->frames.back().evaluation);
    const auto json = evaluation::serialize_evaluation_json(*sweep_, "desktop-obj-pair-v1");
    const auto screenshot = qEnvironmentVariable("OCCLUSION_DESKTOP_SMOKE_IMAGE");
    if (!screenshot.isEmpty() && !scene_image_.save(screenshot))
      return false;
    std::set<QRgb> colors;
    for (int y = 0; y < scene_image_.height(); y += 20)
      for (int x = 0; x < scene_image_.width(); x += 20)
        colors.insert(scene_image_.pixel(x, y));
    return colors.size() > 4 && json.find("\"requestedFrameCount\":11") != std::string::npos;
  }

private:
  static QDoubleSpinBox* spin_box(double minimum, double maximum) {
    auto* spin = new QDoubleSpinBox;
    spin->setRange(minimum, maximum);
    spin->setDecimals(2);
    spin->setSingleStep(1);
    return spin;
  }

  void set_busy(bool busy) {
    import_->setEnabled(!busy);
    evaluate_->setEnabled(!busy && session_.has_value());
    sweep_button_->setEnabled(!busy && session_.has_value());
    frame_slider_->setEnabled(!busy && sweep_.has_value());
    export_->setEnabled(!busy && (pose_.has_value() || sweep_.has_value()));
  }

  bool load_pair(const std::filesystem::path& directory, std::string_view fixed,
                 std::string_view moving) {
    mesh_io::ObjFileError problem;
    auto loaded = evaluation::load_mesh_evaluation_session(directory, fixed, moving, problem);
    if (!loaded) {
      status_->setText(error_text(problem) + ". Last valid scene retained.");
      return false;
    }
    ++generation_;
    session_ = std::move(*loaded);
    pose_.reset();
    sweep_.reset();
    frame_slider_->setEnabled(false);
    renderer_->RemoveAllViewProps();
    add_axes();
    fixed_actor_ = mesh_actor(session_->fixed_mesh, {0.25, 0.65, 0.95});
    moving_actor_ = mesh_actor(session_->moving_mesh, {0.95, 0.58, 0.25});
    renderer_->AddActor(fixed_actor_);
    renderer_->AddActor(moving_actor_);
    renderer_->ResetCamera();
    render_scene();
    status_->setText("Pair imported. Evaluate a pose or sweep.");
    set_busy(false);
    return true;
  }

  void import_pair() {
    load_pair(std::filesystem::path{directory_->text().toStdString()},
              fixed_name_->text().toStdString(), moving_name_->text().toStdString());
  }

  void evaluate_pose() {
    if (!session_)
      return;
    set_busy(true);
    status_->setText("Evaluating pose…");
    const core::MandibularPose pose{core::to_meters(core::Millimeters{opening_->value()}),
                                    core::to_meters(core::Millimeters{protrusion_->value()}),
                                    core::to_meters(core::Millimeters{lateral_->value()})};
    const auto evaluator = session_->evaluator;
    const auto generation = generation_;
    auto* watcher =
        new QFutureWatcher<core::ValidationResult<evaluation::PoseEvaluationResult>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation] {
      const auto result = watcher->result();
      watcher->deleteLater();
      if (generation != generation_)
        return;
      if (result) {
        pose_ = result.value();
        sweep_.reset();
        show_result(*pose_);
      } else {
        status_->setText("Pose is outside the validated native domain.");
      }
      set_busy(false);
    });
    watcher->setFuture(QtConcurrent::run([evaluator, pose] { return evaluator.evaluate(pose); }));
  }

  void evaluate_sweep() {
    if (!session_)
      return;
    set_busy(true);
    status_->setText("Evaluating sweep…");
    const auto preset = static_cast<core::SweepPreset>(preset_->currentData().toInt());
    const auto count = static_cast<std::size_t>(frames_->value());
    const auto evaluator = session_->evaluator;
    const auto generation = generation_;
    auto* watcher =
        new QFutureWatcher<core::ValidationResult<evaluation::EvaluatedSweepResult>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation] {
      const auto result = watcher->result();
      watcher->deleteLater();
      if (generation != generation_)
        return;
      if (result) {
        sweep_ = result.value();
        pose_.reset();
        frame_slider_->setRange(0, static_cast<int>(sweep_->frames.size()) - 1);
        frame_slider_->setValue(0);
        show_result(sweep_->frames.front().evaluation);
      } else {
        status_->setText("Sweep request is outside the validated native domain.");
      }
      set_busy(false);
    });
    watcher->setFuture(QtConcurrent::run(
        [evaluator, preset, count] { return evaluator.evaluate_sweep(preset, count); }));
  }

  void add_axes() {
    const std::array<std::array<double, 3>, 3> directions{{{0.2, 0, 0}, {0, 0.2, 0}, {0, 0, 0.2}}};
    const std::array<std::array<double, 3>, 3> colors{
        {{1, 0.25, 0.25}, {0.25, 1, 0.25}, {0.3, 0.5, 1}}};
    for (std::size_t index = 0; index < directions.size(); ++index) {
      auto line = vtkSmartPointer<vtkLineSource>::New();
      line->SetPoint1(0, 0, 0);
      line->SetPoint2(directions[index].data());
      auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
      mapper->SetInputConnection(line->GetOutputPort());
      auto actor = vtkSmartPointer<vtkActor>::New();
      actor->SetMapper(mapper);
      actor->GetProperty()->SetColor(colors[index][0], colors[index][1], colors[index][2]);
      actor->GetProperty()->SetLineWidth(3);
      renderer_->AddActor(actor);
    }
  }

  void show_result(const evaluation::PoseEvaluationResult& result) {
    auto matrix = vtkSmartPointer<vtkMatrix4x4>::New();
    for (int row = 0; row < 3; ++row)
      for (int column = 0; column < 3; ++column)
        matrix->SetElement(
            row, column,
            result.applied_transform.rotation[static_cast<std::size_t>(row * 3 + column)]);
    const auto& translation = result.applied_transform.translation_meters;
    matrix->SetElement(0, 3, translation.x);
    matrix->SetElement(1, 3, translation.y);
    matrix->SetElement(2, 3, translation.z);
    moving_actor_->SetUserMatrix(matrix);
    for (const auto& actor : contact_actors_)
      renderer_->RemoveActor(actor);
    contact_actors_.clear();
    for (const auto& contact : result.contacts) {
      auto sphere = vtkSmartPointer<vtkSphereSource>::New();
      sphere->SetCenter(contact.position_meters.x, contact.position_meters.y,
                        contact.position_meters.z);
      sphere->SetRadius(0.0015);
      auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
      mapper->SetInputConnection(sphere->GetOutputPort());
      auto actor = vtkSmartPointer<vtkActor>::New();
      actor->SetMapper(mapper);
      actor->GetProperty()->SetColor(1, 0.95, 0.1);
      renderer_->AddActor(actor);
      contact_actors_.push_back(actor);
    }
    QString measurement = "Clearance unavailable";
    if (result.clearance)
      measurement = QString("Clearance %1 mm").arg(result.clearance->value() * 1000, 0, 'f', 3);
    status_->setText(QString("%1 — %2; penetration %3 mm; %4 contact samples")
                         .arg(classification_text(result.classification), measurement)
                         .arg(result.penetration_depth.value() * 1000, 0, 'f', 3)
                         .arg(result.normalized_contact_count));
    render_scene();
  }

  void render_scene() {
    window_->Render();
    auto capture = vtkSmartPointer<vtkWindowToImageFilter>::New();
    capture->SetInput(window_);
    capture->SetInputBufferTypeToRGBA();
    capture->ReadFrontBufferOff();
    capture->Update();
    auto* image = capture->GetOutput();
    auto* scalars = vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetScalars());
    if (!scalars || scalars->GetNumberOfComponents() != 4) {
      status_->setText("VTK could not capture the scene. Check the display/OpenGL setup.");
      return;
    }
    const int* dimensions = image->GetDimensions();
    scene_image_ = QImage(scalars->GetPointer(0), dimensions[0], dimensions[1], dimensions[0] * 4,
                          QImage::Format_RGBA8888)
                       .copy()
                       .mirrored();
    scene_->setPixmap(QPixmap::fromImage(scene_image_)
                          .scaled(scene_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }

  void rotate_camera(double degrees) {
    renderer_->GetActiveCamera()->Azimuth(degrees);
    renderer_->ResetCameraClippingRange();
    render_scene();
  }

  void export_json() {
    if (!pose_ && !sweep_)
      return;
    const auto path =
        QFileDialog::getSaveFileName(this, "Export native evaluation", {}, "JSON (*.json)");
    if (path.isEmpty())
      return;
    const auto json = sweep_ ? evaluation::serialize_evaluation_json(*sweep_, "desktop-obj-pair-v1")
                             : evaluation::serialize_evaluation_json(*pose_, "desktop-obj-pair-v1");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(json.data(), static_cast<qint64>(json.size())) !=
            static_cast<qint64>(json.size()) ||
        !file.commit()) {
      QMessageBox::warning(this, "Export failed", "Could not write the JSON export.");
      return;
    }
    status_->setText("Exported the engine's versioned JSON result.");
  }

  QLineEdit* directory_ = nullptr;
  QLineEdit* fixed_name_ = nullptr;
  QLineEdit* moving_name_ = nullptr;
  QDoubleSpinBox* opening_ = nullptr;
  QDoubleSpinBox* protrusion_ = nullptr;
  QDoubleSpinBox* lateral_ = nullptr;
  QComboBox* preset_ = nullptr;
  QSpinBox* frames_ = nullptr;
  QSlider* frame_slider_ = nullptr;
  QPushButton* import_ = nullptr;
  QPushButton* evaluate_ = nullptr;
  QPushButton* sweep_button_ = nullptr;
  QPushButton* export_ = nullptr;
  QLabel* status_ = nullptr;
  QLabel* scene_ = nullptr;
  QImage scene_image_;
  std::uint64_t generation_ = 0;
  std::optional<evaluation::MeshEvaluationSession> session_;
  std::optional<evaluation::PoseEvaluationResult> pose_;
  std::optional<evaluation::EvaluatedSweepResult> sweep_;
  vtkSmartPointer<vtkRenderer> renderer_;
  vtkSmartPointer<vtkRenderWindow> window_;
  vtkSmartPointer<vtkActor> fixed_actor_;
  vtkSmartPointer<vtkActor> moving_actor_;
  std::vector<vtkSmartPointer<vtkActor>> contact_actors_;
};
} // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  if (argc == 3 && std::string_view{argv[1]} == "--smoke") {
    MainWindow window;
    if (!window.smoke(argv[2])) {
      std::cerr << "occlusion-desktop smoke: FAIL\n";
      return 1;
    }
    std::cout << "occlusion-desktop smoke: PASS\n";
    return 0;
  }
  MainWindow window(argc == 3 && std::string_view{argv[1]} == "--open"
                        ? QString::fromLocal8Bit(argv[2])
                        : QString{});
  window.show();
  return app.exec();
}
