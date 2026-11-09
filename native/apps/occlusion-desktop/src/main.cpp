#include "occlusion/evaluation/mesh_session.hpp"

#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
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
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindowToImageFilter.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {
namespace core = occlusion::core;
namespace evaluation = occlusion::evaluation;
namespace mesh_io = occlusion::mesh_io;

vtkSmartPointer<vtkActor> mesh_actor(const mesh_io::TriangleMesh& mesh,
                                     const std::array<double, 3>& color) {
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
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
  auto normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  normals->SetInputData(polydata);
  normals->SplittingOff();
  mapper->SetInputConnection(normals->GetOutputPort());
  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(color[0], color[1], color[2]);
  actor->GetProperty()->SetOpacity(1.0);
  actor->GetProperty()->SetSpecular(0.3);
  actor->GetProperty()->SetSpecularPower(24);
  return actor;
}

QString error_text(const mesh_io::ObjFileError& error) {
  return QString::fromStdString(error.filename.empty() ? error.field + ": " + error.message
                                                       : mesh_io::describe_obj_file_error(error));
}

QString classification_text(core::ContactClassification value) {
  switch (value) {
  case core::ContactClassification::separated:
    return "Separadas";
  case core::ContactClassification::touching:
    return "En contacto";
  case core::ContactClassification::penetrating:
    return "Intersección";
  }
  return "Sin resultado";
}

struct LoadedPair final {
  std::optional<evaluation::MeshEvaluationSession> session;
  mesh_io::ObjFileError error;
};

class SceneLabel final : public QLabel {
public:
  std::function<void(double, double)> orbit;
  std::function<void(double)> zoom;
  std::function<void()> resized;

  void set_image(QImage image) {
    image_ = std::move(image);
    update_pixmap();
  }

protected:
  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton) {
      last_position_ = event->position();
      setCursor(Qt::ClosedHandCursor);
      event->accept();
    }
  }
  void mouseMoveEvent(QMouseEvent* event) override {
    if ((event->buttons() & Qt::LeftButton) && orbit) {
      const auto delta = event->position() - last_position_;
      last_position_ = event->position();
      orbit(delta.x() * 0.4, delta.y() * 0.4);
      event->accept();
    }
  }
  void mouseReleaseEvent(QMouseEvent* event) override {
    setCursor(Qt::OpenHandCursor);
    event->accept();
  }
  void wheelEvent(QWheelEvent* event) override {
    if (zoom && event->angleDelta().y() != 0)
      zoom(event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15);
    event->accept();
  }
  void resizeEvent(QResizeEvent* event) override {
    QLabel::resizeEvent(event);
    update_pixmap();
    if (resized)
      resized();
  }

private:
  void update_pixmap() {
    if (!image_.isNull())
      setPixmap(
          QPixmap::fromImage(image_).scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }

  QImage image_;
  QPointF last_position_;
};

class MainWindow final : public QWidget {
public:
  explicit MainWindow(QString initial_directory = {}, bool autostart = true) {
    setWindowTitle("Occlusion Lab · Aula de oclusión");
    resize(1280, 820);
    setStyleSheet(R"qss(
      QWidget { background: #f3f6fa; color: #23384b; font-family: "Segoe UI", "Arial"; font-size: 13px; }
      QLabel { background: transparent; }
      QLabel#brand { font-size: 23px; font-weight: 700; color: #133b48; }
      QLabel#muted { color: #61788a; }
      QLabel#section { font-size: 16px; font-weight: 600; }
      QLabel#metric { font-size: 21px; font-weight: 600; color: #126c69; }
      QGroupBox { background: white; border: 1px solid #dce5ec; border-radius: 10px; margin-top: 14px; padding: 16px 10px 10px; font-weight: 600; }
      QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; }
      QPushButton { background: white; border: 1px solid #cddbe3; border-radius: 7px; padding: 9px 12px; font-weight: 600; }
      QPushButton:hover { background: #e6f4f1; border-color: #68aaa2; }
      QPushButton:pressed { background: #cde7e1; }
      QPushButton:disabled { color: #91a1ac; background: #edf1f4; border-color: #e1e6eb; }
      QPushButton#primary { color: white; background: #137e77; border: 1px solid #137e77; }
      QPushButton#primary:hover { background: #09675f; }
      QPushButton#primary:disabled { background: #96b6b4; border-color: #96b6b4; }
      QLineEdit, QDoubleSpinBox, QSpinBox, QComboBox { background: white; border: 1px solid #cddbe3; border-radius: 5px; padding: 6px; min-height: 20px; }
      QLineEdit:focus, QDoubleSpinBox:focus { border-color: #137e77; }
      QTabWidget::pane { border: 0; }
      QTabBar::tab { padding: 10px 12px; background: #e8eef3; color: #526c7d; }
      QTabBar::tab:selected { background: white; color: #126c69; border-bottom: 3px solid #137e77; }
      QScrollArea { border: 0; }
      QSlider::groove:horizontal { background: #cedee4; height: 5px; border-radius: 2px; }
      QSlider::handle:horizontal:disabled { background: #b7c5cc; }
      QSlider::handle:horizontal { background: #137e77; width: 16px; margin: -6px 0; border-radius: 8px; }
    )qss");

    directory_ = new QLineEdit(std::move(initial_directory));
    fixed_name_ = new QLineEdit("fixed-box.obj");
    moving_name_ = new QLineEdit("moving-box.obj");
    browse_ = new QPushButton("Elegir carpeta…");
    import_ = new QPushButton("Importar mallas");
    opening_ = spin_box(0, 250);
    protrusion_ = spin_box(0, 50);
    lateral_ = spin_box(-50, 50);
    evaluate_ = new QPushButton("Evaluar contactos");
    preset_ = new QComboBox;
    preset_->addItem("Cierre", static_cast<int>(core::SweepPreset::closing));
    preset_->addItem("Protrusión", static_cast<int>(core::SweepPreset::protrusive));
    preset_->addItem("Lateral izquierdo", static_cast<int>(core::SweepPreset::left_lateral));
    preset_->addItem("Lateral derecho", static_cast<int>(core::SweepPreset::right_lateral));
    frames_ = new QSpinBox;
    frames_->setRange(2, 61);
    frames_->setValue(31);
    sweep_button_ = new QPushButton("Calcular barrido");
    frame_slider_ = new QSlider(Qt::Horizontal);
    frame_slider_->setEnabled(false);
    frame_label_ = new QLabel("Sin barrido calculado");
    summary_ = new QLabel;
    summary_->setWordWrap(true);
    export_ = new QPushButton("Exportar resultados");
    export_->setEnabled(false);
    status_ = new QLabel("Preparando tu espacio de práctica…");
    status_->setWordWrap(true);
    scene_ = new SceneLabel;
    scene_->setMinimumSize(320, 260);
    scene_->setCursor(Qt::OpenHandCursor);
    scene_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    scene_->setAlignment(Qt::AlignCenter);
    scene_->setStyleSheet("background: #eaf2f6; border-radius: 12px; color: #456273;");
    scene_->setText("Cargando material de práctica…");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 16, 22, 16);
    layout->setSpacing(14);
    auto* header = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    auto* brand = new QLabel("Occlusion Lab");
    brand->setObjectName("brand");
    titles->addWidget(brand);
    auto* subtitle = new QLabel("AULA DE OCLUSIÓN  /  Explora, observa y compara");
    subtitle->setObjectName("muted");
    titles->addWidget(subtitle);
    header->addLayout(titles);
    header->addStretch();
    demo_button_ = new QPushButton("Restablecer ejemplo");
    header->addWidget(demo_button_);
    header->addWidget(export_);
    layout->addLayout(header);
    auto* body = new QHBoxLayout;
    body->setSpacing(18);
    layout->addLayout(body, 1);
    auto* sidebar = new QWidget;
    sidebar->setMinimumWidth(300);
    sidebar->setMaximumWidth(340);
    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(0, 0, 0, 0);
    auto* material = new QGroupBox("MATERIAL DE PRÁCTICA");
    auto* material_layout = new QVBoxLayout(material);
    case_label_ = new QLabel("Arcadas esquemáticas · 14 + 14 piezas");
    case_label_->setWordWrap(true);
    material_layout->addWidget(case_label_);
    auto* disclaimer = new QLabel("Modelo sintético para explorar movimientos. No reproduce "
                                  "anatomía ni permite diagnóstico.");
    disclaimer->setObjectName("muted");
    disclaimer->setWordWrap(true);
    material_layout->addWidget(disclaimer);
    side->addWidget(material);
    auto* tabs = new QTabWidget;
    side->addWidget(tabs, 1);
    auto add_tab = [tabs](const QString& name) {
      auto* scroll = new QScrollArea;
      scroll->setWidgetResizable(true);
      auto* page = new QWidget;
      auto* column = new QVBoxLayout(page);
      column->setContentsMargins(6, 14, 6, 8);
      column->setSpacing(12);
      scroll->setWidget(page);
      tabs->addTab(scroll, name);
      return column;
    };
    auto* explore = add_tab("Explorar");
    auto* guide = new QLabel("1. Cambia la posición.\n2. Evalúa para consultar contactos.\n3. Gira "
                             "la escena y compara.");
    guide->setWordWrap(true);
    guide->setObjectName("muted");
    explore->addWidget(guide);
    auto* pose_form = new QFormLayout;
    pose_form->addRow("Apertura (mm)", opening_);
    pose_form->addRow("Protrusión (mm)", protrusion_);
    pose_form->addRow("Lateralidad (mm)", lateral_);
    explore->addLayout(pose_form);
    auto* examples = new QHBoxLayout;
    auto* open_pose = new QPushButton("Abrir");
    auto* close_pose = new QPushButton("Cerrar");
    auto* protrude_pose = new QPushButton("Protruir");
    pose_examples_ = {open_pose, close_pose, protrude_pose};
    for (auto* button : pose_examples_)
      examples->addWidget(button);
    explore->addLayout(examples);
    evaluate_->setObjectName("primary");
    explore->addWidget(evaluate_);
    auto* lesson = new QGroupBox("¿QUÉ ESTÁS OBSERVANDO?");
    auto* lesson_layout = new QVBoxLayout(lesson);
    auto* lesson_text =
        new QLabel("Apertura: separa las arcadas.\nProtrusión: desplaza la inferior hacia "
                   "delante.\nLateralidad: desplaza la inferior hacia los lados.\n\nLos puntos "
                   "amarillos indican contactos geométricos, no fuerzas.");
    lesson_text->setWordWrap(true);
    lesson_layout->addWidget(lesson_text);
    explore->addWidget(lesson);
    explore->addStretch();
    auto* sweep_tab = add_tab("Barrido");
    auto* sweep_help =
        new QLabel("Compara una secuencia de posiciones y reproduce sus resultados. Los recorridos "
                   "usan los rangos completos del modelo sintético; no son límites fisiológicos.");
    sweep_help->setWordWrap(true);
    sweep_tab->addWidget(sweep_help);
    auto* sweep_form = new QFormLayout;
    sweep_form->addRow("Movimiento", preset_);
    sweep_form->addRow("Posiciones", frames_);
    sweep_tab->addLayout(sweep_form);
    sweep_button_->setObjectName("primary");
    sweep_tab->addWidget(sweep_button_);
    sweep_tab->addWidget(summary_);
    sweep_tab->addStretch();
    auto* import_tab = add_tab("Importar");
    auto* import_help =
        new QLabel("Opcional: carga dos mallas OBJ sintéticas cerradas, con coordenadas en metros. "
                   "El ejemplo integrado siempre está disponible arriba.");
    import_help->setWordWrap(true);
    import_tab->addWidget(import_help);
    import_tab->addWidget(new QLabel("Carpeta de los archivos"));
    import_tab->addWidget(directory_);
    import_tab->addWidget(browse_);
    import_tab->addWidget(new QLabel("Malla fija (superior)"));
    import_tab->addWidget(fixed_name_);
    import_tab->addWidget(new QLabel("Malla móvil (inferior)"));
    import_tab->addWidget(moving_name_);
    import_tab->addWidget(import_);
    import_tab->addStretch();
    body->addWidget(sidebar);
    auto* workspace = new QVBoxLayout;
    body->addLayout(workspace, 1);
    auto* scene_header = new QHBoxLayout;
    auto* scene_title = new QLabel("Exploración 3D");
    scene_title->setObjectName("section");
    scene_header->addWidget(scene_title);
    scene_header->addStretch();
    scene_header->addWidget(new QLabel("● Superior: marfil     ● Inferior: turquesa"));
    workspace->addLayout(scene_header);
    workspace->addWidget(scene_, 1);
    auto* camera_row = new QHBoxLayout;
    auto* rotate_left = new QPushButton("↶");
    auto* rotate_right = new QPushButton("↷");
    auto* rotate_up = new QPushButton("↑");
    auto* rotate_down = new QPushButton("↓");
    auto* zoom_in = new QPushButton("+");
    auto* zoom_out = new QPushButton("−");
    auto* reset_view = new QPushButton("Centrar vista");
    camera_row->addWidget(new QLabel("Arrastra para girar · Rueda para zoom"), 1);
    for (auto* button :
         {rotate_left, rotate_right, rotate_up, rotate_down, zoom_in, zoom_out, reset_view})
      camera_row->addWidget(button);
    workspace->addLayout(camera_row);
    auto* timeline = new QHBoxLayout;
    play_ = new QPushButton("Reproducir");
    play_->setEnabled(false);
    timeline->addWidget(play_);
    timeline->addWidget(frame_slider_, 1);
    timeline->addWidget(frame_label_);
    workspace->addLayout(timeline);
    auto* metrics = new QHBoxLayout;
    auto metric = [metrics](const QString& title, QLabel*& value) {
      auto* card = new QGroupBox(title);
      auto* column = new QVBoxLayout(card);
      value = new QLabel("—");
      value->setObjectName("metric");
      column->addWidget(value);
      metrics->addWidget(card, 1);
    };
    metric("RELACIÓN ENTRE MALLAS", relation_);
    metric("SEPARACIÓN", clearance_);
    metric("MUESTRAS DE CONTACTO", contacts_);
    workspace->addLayout(metrics);
    workspace->addWidget(status_);
    playback_ = new QTimer(this);
    playback_->setInterval(140);
    connect(playback_, &QTimer::timeout, this, [this] {
      if (!sweep_ || busy_ || frame_slider_->value() >= frame_slider_->maximum()) {
        stop_playback();
        return;
      }
      frame_slider_->setValue(frame_slider_->value() + 1);
    });
    connect(play_, &QPushButton::clicked, this, [this] {
      if (playback_->isActive()) {
        stop_playback();
        return;
      }
      if (frame_slider_->value() == frame_slider_->maximum())
        frame_slider_->setValue(0);
      play_->setText("Pausar");
      playback_->start();
    });
    connect(demo_button_, &QPushButton::clicked, this, [this] { load_demo(); });
    connect(reset_view, &QPushButton::clicked, this, [this] { reset_camera(); });
    connect(open_pose, &QPushButton::clicked, this, [this] { example_pose(8, 0); });
    connect(close_pose, &QPushButton::clicked, this, [this] { example_pose(0, 0); });
    connect(protrude_pose, &QPushButton::clicked, this, [this] { example_pose(2, 4); });
    scene_->orbit = [this](double x, double y) {
      renderer_->GetActiveCamera()->Azimuth(-x);
      renderer_->GetActiveCamera()->Elevation(y);
      renderer_->GetActiveCamera()->OrthogonalizeViewUp();
      renderer_->ResetCameraClippingRange();
      render_scene();
    };
    scene_->zoom = [this](double factor) { zoom_camera(factor); };
    scene_->resized = [this] {
      if (window_) {
        window_->SetSize(std::max(1, scene_->width()), std::max(1, scene_->height()));
        render_scene();
      }
    };
    rotate_left->setToolTip("Girar a la izquierda");
    rotate_right->setToolTip("Girar a la derecha");
    rotate_up->setToolTip("Girar hacia arriba");
    rotate_down->setToolTip("Girar hacia abajo");
    zoom_in->setToolTip("Acercar");
    zoom_out->setToolTip("Alejar");
    for (auto* button : {rotate_left, rotate_right, rotate_up, rotate_down, zoom_in, zoom_out})
      button->setAccessibleName(button->toolTip());

    renderer_ = vtkSmartPointer<vtkRenderer>::New();
    renderer_->SetBackground(0.82, 0.89, 0.92);
    renderer_->SetBackground2(0.97, 0.98, 0.99);
    renderer_->GradientBackgroundOn();
    window_ = vtkSmartPointer<vtkRenderWindow>::New();
    window_->SetOffScreenRendering(1);
    window_->SetSize(800, 600);
    window_->AddRenderer(renderer_);
    add_axes();
    set_busy(false);

    connect(browse_, &QPushButton::clicked, this, [this] {
      const auto selected = QFileDialog::getExistingDirectory(
          this, "Selecciona la carpeta de las mallas", directory_->text());
      if (!selected.isEmpty())
        directory_->setText(selected);
    });
    connect(import_, &QPushButton::clicked, this, [this] { import_pair(); });
    connect(evaluate_, &QPushButton::clicked, this, [this] { evaluate_pose(); });
    connect(sweep_button_, &QPushButton::clicked, this, [this] { evaluate_sweep(); });
    connect(frame_slider_, &QSlider::valueChanged, this, [this](int index) {
      if (sweep_ && index < static_cast<int>(sweep_->frames.size())) {
        frame_label_->setText(QString("Posición %1/%2").arg(index + 1).arg(sweep_->frames.size()));
        show_result(sweep_->frames[static_cast<std::size_t>(index)].evaluation);
      }
    });
    connect(export_, &QPushButton::clicked, this, [this] { export_json(); });
    connect(rotate_left, &QPushButton::clicked, this, [this] { rotate_camera(-15); });
    connect(rotate_right, &QPushButton::clicked, this, [this] { rotate_camera(15); });
    connect(rotate_up, &QPushButton::clicked, this, [this] { elevate_camera(15); });
    connect(rotate_down, &QPushButton::clicked, this, [this] { elevate_camera(-15); });
    connect(zoom_in, &QPushButton::clicked, this, [this] { zoom_camera(1.2); });
    connect(zoom_out, &QPushButton::clicked, this, [this] { zoom_camera(1.0 / 1.2); });
    for (auto* field : {directory_, fixed_name_, moving_name_})
      connect(field, &QLineEdit::textChanged, this, [this] { selection_changed(); });
    for (auto* field : {opening_, protrusion_, lateral_})
      connect(field, &QDoubleSpinBox::valueChanged, this, [this] { pose_inputs_changed(); });
    connect(preset_, &QComboBox::currentIndexChanged, this, [this] { sweep_inputs_changed(); });
    connect(frames_, &QSpinBox::valueChanged, this, [this] { sweep_inputs_changed(); });
    if (autostart) {
      if (directory_->text().isEmpty())
        load_demo();
      else
        import_pair(true);
    }
  }

  bool smoke(const std::filesystem::path& directory) {
    directory_->setText(QString::fromStdString(directory.string()));
    import_->click();
    if (directory_->isEnabled() || fixed_name_->isEnabled() || browse_->isEnabled())
      return false;
    if (!wait_until_ready(import_) || !session_)
      return false;
    fixed_name_->setText("missing.obj");
    import_->click();
    if (!wait_until_ready(import_) || session_ || evaluate_->isEnabled() ||
        !status_->text().contains("missing.obj"))
      return false;
    fixed_name_->setText("fixed-box.obj");
    import_->click();
    if (!wait_until_ready(import_) || !session_)
      return false;
    moving_name_->setText("other.obj");
    if (evaluate_->isEnabled() || sweep_button_->isEnabled() || export_->isEnabled())
      return false;
    moving_name_->setText("moving-box.obj");
    if (!evaluate_->isEnabled())
      return false;
    opening_->setValue(250);
    evaluate_->click();
    import_pair(); // Simulate a newer selection before the earlier worker result is published.
    if (!wait_until_ready(import_) || !QThreadPool::globalInstance()->waitForDone(5000))
      return false;
    QCoreApplication::processEvents();
    if (pose_ || !evaluate_->isEnabled())
      return false;
    evaluate_->click();
    if (opening_->isEnabled() || protrusion_->isEnabled() || lateral_->isEnabled())
      return false;
    if (!wait_until_ready(evaluate_) || !pose_ ||
        pose_->classification != core::ContactClassification::separated)
      return false;
    opening_->setValue(1);
    if (pose_ || export_->isEnabled())
      return false;
    opening_->setValue(0);
    evaluate_->click();
    if (!wait_until_ready(evaluate_) || !pose_)
      return false;
    frames_->setValue(11);
    sweep_button_->click();
    if (preset_->isEnabled() || frames_->isEnabled())
      return false;
    if (!wait_until_ready(sweep_button_) || !sweep_ || sweep_->frames.size() != 11 ||
        sweep_->frames.front().evaluation.classification !=
            core::ContactClassification::separated ||
        sweep_->frames.back().evaluation.classification != core::ContactClassification::touching ||
        !sweep_->summary.contact_persists_through_final_frame)
      return false;
    frame_slider_->setValue(10);
    if (frame_label_->text() != "Posición 11/11" || !status_->text().startsWith("En contacto"))
      return false;
    const auto& selected_transform = sweep_->frames[10].evaluation.applied_transform;
    const auto* selected_matrix = moving_actor_->GetUserMatrix();
    for (int row = 0; row < 3; ++row) {
      for (int column = 0; column < 3; ++column) {
        if (selected_matrix->GetElement(row, column) !=
            selected_transform.rotation[static_cast<std::size_t>(row * 3 + column)])
          return false;
      }
    }
    if (selected_matrix->GetElement(0, 3) != selected_transform.translation_meters.x ||
        selected_matrix->GetElement(1, 3) != selected_transform.translation_meters.y ||
        selected_matrix->GetElement(2, 3) != selected_transform.translation_meters.z)
      return false;
    const auto json = evaluation::serialize_evaluation_json(*sweep_, "desktop-obj-pair-v1");
    QTemporaryDir temporary;
    if (!temporary.isValid() || !write_export(temporary.filePath("evaluation.json")))
      return false;
    QFile exported(temporary.filePath("evaluation.json"));
    if (!exported.open(QIODevice::ReadOnly) ||
        exported.readAll() != QByteArray::fromStdString(json))
      return false;
    frames_->setValue(12);
    if (sweep_ || export_->isEnabled() || frame_slider_->isEnabled())
      return false;
    frames_->setValue(11);
    sweep_button_->click();
    if (!wait_until_ready(sweep_button_) || !sweep_ ||
        evaluation::serialize_evaluation_json(*sweep_, "desktop-obj-pair-v1") != json)
      return false;
    frame_slider_->setValue(10);
    const auto screenshot = qEnvironmentVariable("OCCLUSION_DESKTOP_SMOKE_IMAGE");
    if (!screenshot.isEmpty() && !scene_image_.save(screenshot))
      return false;
    const auto window_screenshot = qEnvironmentVariable("OCCLUSION_DESKTOP_SMOKE_WINDOW");
    if (!window_screenshot.isEmpty()) {
      show();
      resize(1360, 840);
      QCoreApplication::processEvents();
      if (!grab().save(window_screenshot))
        return false;
    }
    std::set<QRgb> colors;
    for (int y = 0; y < scene_image_.height(); y += 10)
      for (int x = 0; x < scene_image_.width(); x += 10)
        colors.insert(scene_image_.pixel(x, y));
    // Exercise engine rejection even though normal UI ranges prevent these requests.
    opening_->setMaximum(300);
    opening_->setValue(300);
    evaluate_->click();
    if (!wait_until_ready(evaluate_) || pose_ || sweep_ || export_->isEnabled() ||
        frame_slider_->isEnabled())
      return false;
    opening_->setMaximum(250);
    opening_->setValue(0);
    evaluate_->click();
    if (!wait_until_ready(evaluate_) || !pose_ || !export_->isEnabled())
      return false;
    frames_->setMaximum(62);
    frames_->setValue(62);
    sweep_button_->click();
    if (!wait_until_ready(sweep_button_) || pose_ || sweep_ || export_->isEnabled())
      return false;
    frames_->setMaximum(61);
    frames_->setValue(11);
    sweep_button_->click();
    if (!wait_until_ready(sweep_button_) || !sweep_ || !export_->isEnabled())
      return false;
    return colors.size() > 4 && json.find("\"requestedFrameCount\":11") != std::string::npos;
  }

  bool smoke_startup() {
    if (!wait_until_ready(import_) || !session_ || !pose_ || !export_->isEnabled() ||
        session_->fixed_mesh.vertices_meters.size() != 868 ||
        pose_->classification != core::ContactClassification::separated ||
        std::abs(pose_->requested_pose.opening.value() - 0.008) > 1e-12)
      return false;
    QTemporaryDir output;
    if (!output.isValid() || !write_export(output.filePath("demo.json")))
      return false;
    QFile file(output.filePath("demo.json"));
    if (!file.open(QIODevice::ReadOnly) || !file.readAll().contains("desktop-teaching-arches-v1"))
      return false;
    const auto previous_x = renderer_->GetActiveCamera()->GetPosition()[0];
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent move(QEvent::MouseMove, QPointF(140, 110), QPointF(140, 110), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(140, 110), QPointF(140, 110),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(scene_, &press);
    QApplication::sendEvent(scene_, &move);
    QApplication::sendEvent(scene_, &release);
    if (renderer_->GetActiveCamera()->GetPosition()[0] == previous_x)
      return false;
    const auto distance = renderer_->GetActiveCamera()->GetDistance();
    QWheelEvent wheel(QPointF(100, 100), QPointF(100, 100), QPoint(), QPoint(0, 120), Qt::NoButton,
                      Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(scene_, &wheel);
    if (renderer_->GetActiveCamera()->GetDistance() >= distance)
      return false;
    opening_->setValue(12);
    if (export_->isEnabled() || relation_->text() != "Sin evaluar" ||
        std::abs(moving_actor_->GetUserMatrix()->GetElement(1, 3) - 0.148) > 1e-12)
      return false;
    pose_examples_[1]->click();
    if (!wait_until_ready(evaluate_) || !pose_ || pose_->requested_pose.opening.value() != 0)
      return false;
    frames_->setValue(11);
    sweep_button_->click();
    if (!wait_until_ready(sweep_button_) || !sweep_ || !play_->isEnabled())
      return false;
    play_->click();
    QElapsedTimer timer;
    timer.start();
    while (frame_slider_->value() == 0 && timer.elapsed() < 2000)
      QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    if (frame_slider_->value() == 0)
      return false;
    play_->click();
    if (playback_->isActive())
      return false;
    opening_->setValue(9);
    if (sweep_ || play_->isEnabled() || export_->isEnabled())
      return false;
    demo_button_->click();
    if (!wait_until_ready(import_) || !pose_ || !export_->isEnabled() || opening_->value() != 8)
      return false;
    show();
    QCoreApplication::processEvents();
    const auto screenshot = qEnvironmentVariable("OCCLUSION_DESKTOP_SMOKE_WINDOW");
    if (!screenshot.isEmpty() && !grab().save(screenshot))
      return false;
    return !scene_image_.isNull();
  }

private:
  void stop_playback() {
    playback_->stop();
    play_->setText("Reproducir");
  }

  void clear_metrics() {
    relation_->setText("Sin evaluar");
    clearance_->setText("—");
    contacts_->setText("—");
  }

  void reset_camera() {
    auto* camera = renderer_->GetActiveCamera();
    camera->SetPosition(0.09, 0.055, 0.13);
    camera->SetFocalPoint(0, 0, 0.02);
    camera->SetViewUp(0, 1, 0);
    renderer_->ResetCamera();
    renderer_->ResetCameraClippingRange();
    render_scene();
  }

  void load_demo() {
    if (busy_)
      return;
    if (!demo_directory_.isValid()) {
      status_->setText("No se pudo preparar el ejemplo. Comprueba el espacio temporal disponible.");
      return;
    }
    for (const auto* name : {"upper-arch.obj", "lower-arch.obj"}) {
      const auto destination = demo_directory_.filePath(name);
      if (!QFile::exists(destination) && !QFile::copy(QString(":/demo/") + name, destination)) {
        status_->setText("No se pudo cargar el material integrado.");
        return;
      }
    }
    directory_->setText(demo_directory_.path());
    fixed_name_->setText("upper-arch.obj");
    moving_name_->setText("lower-arch.obj");
    opening_->setValue(8);
    protrusion_->setValue(0);
    lateral_->setValue(0);
    import_pair(true);
  }

  void example_pose(double opening, double protrusion) {
    opening_->setValue(opening);
    protrusion_->setValue(protrusion);
    lateral_->setValue(0);
    reset_after_evaluation_ = true;
    evaluate_pose();
  }

  static bool wait_until_ready(const QPushButton* button) {
    QElapsedTimer timer;
    timer.start();
    while (!button->isEnabled() && timer.elapsed() < 5000)
      QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    return button->isEnabled();
  }

  static QDoubleSpinBox* spin_box(double minimum, double maximum) {
    auto* spin = new QDoubleSpinBox;
    spin->setRange(minimum, maximum);
    spin->setDecimals(2);
    spin->setSingleStep(1);
    return spin;
  }

  bool selection_matches_import() const {
    return session_ && directory_->text() == imported_directory_ &&
           fixed_name_->text() == imported_fixed_name_ &&
           moving_name_->text() == imported_moving_name_;
  }

  void selection_changed() {
    if (!session_)
      return;
    if (!selection_matches_import()) {
      stop_playback();
      clear_metrics();
    }
    status_->setText(
        selection_matches_import()
            ? "Selección restaurada. Evalúa una posición o un barrido."
            : "La selección cambió. Importa las mallas para evaluar; se conserva la última "
              "escena válida.");
    set_busy(busy_);
  }

  void pose_inputs_changed() {
    if (!selection_matches_import() || busy_)
      return;
    clear_evaluation();
    const auto transform = core::mandibular_pose_to_transform(
        {core::to_meters(core::Millimeters{opening_->value()}),
         core::to_meters(core::Millimeters{protrusion_->value()}),
         core::to_meters(core::Millimeters{lateral_->value()})});
    if (transform && moving_actor_) {
      auto matrix = vtkSmartPointer<vtkMatrix4x4>::New();
      const auto& translation = transform.value().translation_meters;
      matrix->SetElement(0, 3, translation.x);
      matrix->SetElement(1, 3, translation.y);
      matrix->SetElement(2, 3, translation.z);
      moving_actor_->SetUserMatrix(matrix);
      for (const auto& actor : contact_actors_)
        renderer_->RemoveActor(actor);
      contact_actors_.clear();
      renderer_->ResetCameraClippingRange();
      render_scene();
    }
    status_->setText(
        "Vista previa de posición · Pulsa Evaluar contactos para actualizar los resultados.");
    set_busy(busy_);
  }

  void sweep_inputs_changed() {
    if (sweep_) {
      stop_playback();
      clear_metrics();
      sweep_.reset();
      frame_label_->setText("Sin barrido calculado");
      summary_->clear();
      status_->setText("El barrido cambió. Vuelve a calcularlo antes de exportar.");
      set_busy(busy_);
    }
  }

  void set_busy(bool busy) {
    busy_ = busy;
    if (busy)
      stop_playback();
    demo_button_->setEnabled(!busy);
    for (auto* button : pose_examples_)
      button->setEnabled(!busy && selection_matches_import());
    play_->setEnabled(!busy && selection_matches_import() && sweep_.has_value());
    browse_->setEnabled(!busy);
    directory_->setEnabled(!busy);
    fixed_name_->setEnabled(!busy);
    moving_name_->setEnabled(!busy);
    opening_->setEnabled(!busy);
    protrusion_->setEnabled(!busy);
    lateral_->setEnabled(!busy);
    preset_->setEnabled(!busy);
    frames_->setEnabled(!busy);
    import_->setEnabled(!busy);
    const bool imported = selection_matches_import();
    evaluate_->setEnabled(!busy && imported);
    sweep_button_->setEnabled(!busy && imported);
    frame_slider_->setEnabled(!busy && imported && sweep_.has_value());
    export_->setEnabled(!busy && imported && (pose_.has_value() || sweep_.has_value()));
  }

  void accept_loaded_pair(LoadedPair&& loaded) {
    if (!loaded.session) {
      session_.reset();
      imported_directory_.clear();
      imported_fixed_name_.clear();
      imported_moving_name_.clear();
      pose_.reset();
      sweep_.reset();
      frame_label_->setText("Sin barrido calculado");
      summary_->clear();
      status_->setText(error_text(loaded.error) +
                       ". Se conserva la última escena; importa un par válido para evaluar.");
      set_busy(false);
      return;
    }
    session_ = std::move(*loaded.session);
    imported_directory_ = directory_->text();
    imported_fixed_name_ = fixed_name_->text();
    imported_moving_name_ = moving_name_->text();
    pose_.reset();
    sweep_.reset();
    frame_label_->setText("Sin barrido calculado");
    summary_->clear();
    frame_slider_->setEnabled(false);
    renderer_->RemoveAllViewProps();
    contact_actors_.clear();
    add_axes();
    fixed_actor_ = mesh_actor(session_->fixed_mesh, {0.96, 0.90, 0.77});
    moving_actor_ = mesh_actor(session_->moving_mesh, {0.17, 0.65, 0.61});
    renderer_->AddActor(fixed_actor_);
    renderer_->AddActor(moving_actor_);
    reset_camera();
    case_label_->setText(directory_->text() == demo_directory_.path()
                             ? "Arcadas esquemáticas · 14 + 14 piezas"
                             : "Par de mallas importadas");
    status_->setText("Mallas listas. Explora una posición o calcula un barrido.");
    set_busy(false);
  }

  void import_pair(bool evaluate_after_import = false) {
    const auto directory = std::filesystem::path{directory_->text().toStdString()};
    const auto fixed = fixed_name_->text().toStdString();
    const auto moving = moving_name_->text().toStdString();
    const auto generation = ++generation_;
    clear_evaluation();
    set_busy(true);
    status_->setText("Cargando material…");
    auto* watcher = new QFutureWatcher<std::shared_ptr<LoadedPair>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this,
            [this, watcher, generation, evaluate_after_import] {
              auto loaded = watcher->result();
              watcher->deleteLater();
              if (generation == generation_) {
                accept_loaded_pair(std::move(*loaded));
                if (session_ && evaluate_after_import) {
                  reset_after_evaluation_ = true;
                  evaluate_pose();
                }
              }
            });
    watcher->setFuture(QtConcurrent::run([directory, fixed, moving] {
      auto loaded = std::make_shared<LoadedPair>();
      loaded->session =
          evaluation::load_mesh_evaluation_session(directory, fixed, moving, loaded->error);
      return loaded;
    }));
  }

  void evaluate_pose() {
    if (!selection_matches_import())
      return;
    clear_evaluation();
    set_busy(true);
    status_->setText("Evaluando contactos…");
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
        frame_label_->setText("Sin barrido calculado");
        summary_->clear();
        show_result(*pose_);
        if (reset_after_evaluation_) {
          reset_after_evaluation_ = false;
          reset_camera();
        }
      } else {
        status_->setText("La posición está fuera del rango del modelo.");
      }
      set_busy(false);
    });
    watcher->setFuture(QtConcurrent::run([evaluator, pose] { return evaluator.evaluate(pose); }));
  }

  void evaluate_sweep() {
    if (!selection_matches_import())
      return;
    clear_evaluation();
    set_busy(true);
    status_->setText("Calculando posiciones del barrido…");
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
        summary_->setText(QString("Posiciones con contacto: %1/%2; penetración máxima: %3 mm")
                              .arg(sweep_->summary.contact_frame_count)
                              .arg(sweep_->summary.total_frame_count)
                              .arg(sweep_->summary.maximum_penetration.value() * 1000, 0, 'f', 3));
        frame_slider_->setRange(0, static_cast<int>(sweep_->frames.size()) - 1);
        frame_slider_->setValue(0);
        frame_label_->setText(QString("Posición 1/%1").arg(sweep_->frames.size()));
        show_result(sweep_->frames.front().evaluation);
      } else {
        status_->setText("El barrido está fuera del rango permitido.");
      }
      set_busy(false);
    });
    watcher->setFuture(QtConcurrent::run(
        [evaluator, preset, count] { return evaluator.evaluate_sweep(preset, count); }));
  }

  void clear_evaluation() {
    stop_playback();
    clear_metrics();
    pose_.reset();
    sweep_.reset();
    frame_slider_->setRange(0, 0);
    frame_label_->setText("Sin barrido calculado");
    summary_->clear();
  }

  void add_axes() {
    const std::array<std::array<double, 3>, 3> directions{
        {{0.02, 0, 0}, {0, 0.02, 0}, {0, 0, 0.02}}};
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
    relation_->setText(classification_text(result.classification));
    clearance_->setText(result.clearance
                            ? QString("%1 mm").arg(result.clearance->value() * 1000, 0, 'f', 2)
                            : "No disponible");
    contacts_->setText(QString::number(result.normalized_contact_count));
    QString measurement = "Separación no disponible";
    if (result.clearance)
      measurement = QString("Separación %1 mm").arg(result.clearance->value() * 1000, 0, 'f', 3);
    status_->setText(QString("%1 · %2 · penetración %3 mm · %4 muestras de contacto")
                         .arg(classification_text(result.classification), measurement)
                         .arg(result.penetration_depth.value() * 1000, 0, 'f', 3)
                         .arg(result.normalized_contact_count));
    if (sweep_)
      renderer_->ResetCamera();
    renderer_->ResetCameraClippingRange();
    render_scene();
  }

  void render_scene() {
    window_->Render();
    auto capture = vtkSmartPointer<vtkWindowToImageFilter>::New();
    capture->SetInput(window_);
    capture->SetInputBufferTypeToRGB();
    capture->ReadFrontBufferOff();
    capture->Update();
    auto* image = capture->GetOutput();
    auto* scalars = vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetScalars());
    if (!scalars || scalars->GetNumberOfComponents() != 3) {
      status_->setText("No se pudo dibujar la escena. Revisa el controlador gráfico.");
      return;
    }
    const int* dimensions = image->GetDimensions();
    scene_image_ = QImage(scalars->GetPointer(0), dimensions[0], dimensions[1], dimensions[0] * 3,
                          QImage::Format_RGB888)
                       .copy()
                       .mirrored();
    scene_->set_image(scene_image_);
  }

  void rotate_camera(double degrees) {
    renderer_->GetActiveCamera()->Azimuth(degrees);
    renderer_->ResetCameraClippingRange();
    render_scene();
  }

  void elevate_camera(double degrees) {
    renderer_->GetActiveCamera()->Elevation(degrees);
    renderer_->GetActiveCamera()->OrthogonalizeViewUp();
    renderer_->ResetCameraClippingRange();
    render_scene();
  }

  void zoom_camera(double factor) {
    renderer_->GetActiveCamera()->Dolly(factor);
    renderer_->ResetCameraClippingRange();
    render_scene();
  }

  void export_json() {
    if (!selection_matches_import() || (!pose_ && !sweep_))
      return;
    const auto path = QFileDialog::getSaveFileName(this, "Guardar resultados", {}, "JSON (*.json)");
    if (path.isEmpty())
      return;
    if (!write_export(path)) {
      QMessageBox::warning(this, "No se pudo exportar",
                           "No se pudo guardar el archivo de resultados.");
      return;
    }
    status_->setText("Resultados guardados en formato JSON.");
  }

  bool write_export(const QString& path) const {
    if (!selection_matches_import() || (!pose_ && !sweep_))
      return false;
    const auto fixture = imported_directory_ == demo_directory_.path() &&
                                 imported_fixed_name_ == "upper-arch.obj" &&
                                 imported_moving_name_ == "lower-arch.obj"
                             ? "desktop-teaching-arches-v1"
                             : "desktop-obj-pair-v1";
    const auto json = sweep_ ? evaluation::serialize_evaluation_json(*sweep_, fixture)
                             : evaluation::serialize_evaluation_json(*pose_, fixture);
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) &&
           file.write(json.data(), static_cast<qint64>(json.size())) ==
               static_cast<qint64>(json.size()) &&
           file.commit();
  }

  QTemporaryDir demo_directory_;
  QPushButton* demo_button_ = nullptr;
  QPushButton* play_ = nullptr;
  QTimer* playback_ = nullptr;
  QLabel* case_label_ = nullptr;
  QLabel* relation_ = nullptr;
  QLabel* clearance_ = nullptr;
  QLabel* contacts_ = nullptr;
  std::vector<QPushButton*> pose_examples_;
  bool reset_after_evaluation_ = false;
  QLineEdit* directory_ = nullptr;
  QLineEdit* fixed_name_ = nullptr;
  QLineEdit* moving_name_ = nullptr;
  QPushButton* browse_ = nullptr;
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
  QLabel* frame_label_ = nullptr;
  QLabel* summary_ = nullptr;
  SceneLabel* scene_ = nullptr;
  QImage scene_image_;
  bool busy_ = false;
  QString imported_directory_;
  QString imported_fixed_name_;
  QString imported_moving_name_;
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
    MainWindow window({}, false);
    if (!window.smoke(argv[2])) {
      std::cerr << "occlusion-desktop smoke: FAIL\n";
      return 1;
    }
    MainWindow startup;
    if (!startup.smoke_startup()) {
      std::cerr << "occlusion-desktop startup smoke: FAIL\n";
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
