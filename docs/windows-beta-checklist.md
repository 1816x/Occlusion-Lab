# Windows beta acceptance check

Download `occlusion-lab-windows-x64-preview` from the passing workflow linked in
[README](../README.md#phase-414-desktop-development-preview). Extract the artifact,
extract the downloaded ZIP once into a new folder, then launch `occlusion-desktop.exe`
from that folder. Keep the executable and DLLs together. No Qt or MSYS2 installation is required.
This preview targets Windows 10/11 on x64 processors and is unsigned.
If Windows reports that it cannot access the executable, use the
[launch check](windows-launch.md).

Record `build-commit.txt`, your Windows version, and your graphics hardware with the results.

| Check | Expected result |
| --- | --- |
| Launch from File Explorer | The Spanish student workspace opens with both built-in schematic arches visible, 8 mm separation, and active controls, without selecting files. |
| Drag the scene and use the mouse wheel | The camera rotates and zooms; Centrar vista restores the view. |
| Use Abrir / Cerrar / Protruir | The lower arch moves and the contact indicators update after evaluation. |
| Edit a pose value | The scene previews the new position; stale results and export are disabled until Evaluar contactos. |
| Restablecer ejemplo | The built-in arches return to 8 mm opening, with a valid evaluated result. |
| Optional: use Importar to load the included box OBJ files | Ivory fixed and turquoise moving meshes appear; evaluation controls become available. |
| Set Opening to 250 mm and evaluate | Separated; clearance approximately 250 mm; zero penetration. |
| Set Opening to 0 mm and evaluate | Touching; zero penetration; clearance unavailable. |
| Run Cierre with 31 positions in Barrido | Starts separated and finishes touching; the slider selects stored frames, and Reproducir / Pausar controls playback. |
| Rotate and zoom with the camera buttons; resize the window | The scene updates and fits its panel. |
| Export JSON v1 | A complete JSON file is saved with 31 frames for the sweep and meter-valued measurements. |
| Change a mesh filename to a missing file and import | A file error appears; evaluation and export are disabled; the retained scene is labeled. |
| Restore the sample filename and import again | Evaluation and export recover after a successful request. |
| Change pose or sweep inputs after evaluating | The corresponding old result cannot be exported until recalculated. |
| Close and reopen the app | The process exits normally and a fresh window opens. |

Report the failed row, exact input, and visible error text. The artifact includes
`windows-smoke.png`, captured during CI's packaged launch. CI verifies the extracted
ZIP with the Windows Qt plugin and no developer toolchain on PATH; it also checks automatic resource loading, mouse controls, live pose preview, playback and reset; the table above
still requires a hands-on check on your PC.
