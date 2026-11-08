# Windows beta acceptance check

Download `occlusion-lab-windows-x64-preview` from the passing workflow linked in
[README](../README.md#phase-414-desktop-development-preview). Extract the artifact,
then extract `occlusion-lab-windows-x64.zip`. Keep the complete `Occlusion Lab`
folder together and launch `occlusion-desktop.exe`. No Qt or MSYS2 installation is required.
This preview targets Windows 10/11 on x64 processors and is unsigned.

Record `build-commit.txt`, your Windows version, and your graphics hardware with the results.

| Check | Expected result |
| --- | --- |
| Launch from File Explorer | A readable window appears without a missing-DLL or Qt platform-plugin error. |
| Import the included sample folder | Blue fixed and orange moving meshes appear; evaluation controls become available. |
| Set Opening to 250 mm and evaluate | Separated; clearance approximately 250 mm; zero penetration. |
| Set Opening to 0 mm and evaluate | Touching; zero penetration; clearance unavailable. |
| Run Closing with 31 frames | Starts separated and finishes touching; the slider selects each stored frame. |
| Rotate and zoom with the camera buttons; resize the window | The scene updates and fits its panel. |
| Export JSON v1 | A complete JSON file is saved with 31 frames for the sweep and meter-valued measurements. |
| Change a mesh filename to a missing file and import | A file error appears; evaluation and export are disabled; the retained scene is labeled. |
| Restore the sample filename and import again | Evaluation and export recover after a successful request. |
| Change pose or sweep inputs after evaluating | The corresponding old result cannot be exported until recalculated. |
| Close and reopen the app | The process exits normally and a fresh window opens. |

Report the failed row, exact input, and visible error text. The artifact includes
`windows-smoke.png`, captured during CI's packaged launch. CI verifies the extracted
ZIP with the Windows Qt plugin and no developer toolchain on PATH; the table above
still requires a hands-on check on your PC.
