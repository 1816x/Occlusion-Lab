# macOS preview acceptance check

Download the matching Apple Silicon or Intel artifact linked in [README](../README.md#phase-414-desktop-development-preview).
Unzip the artifact and its inner app ZIP. Keep the two OBJ files together.
These checks use the bundled synthetic meshes.

Record the artifact run number, Mac model, processor, and macOS version with the results.

| Check | Expected result |
| --- | --- |
| Open the app from Finder | A usable window with readable controls and scene appears. If Gatekeeper blocks this unnotarized preview, use Privacy & Security → Open Anyway. |
| Select the sample folder and import the pair | Blue fixed and orange moving meshes appear; evaluation controls become available. |
| Set Opening to 250 mm and evaluate | Separated; clearance approximately 250 mm; zero penetration. |
| Set Opening to 0 mm and evaluate | Touching; zero penetration; clearance unavailable. |
| Run Closing with 31 frames | Starts separated and finishes touching. Move the slider to compare the first and last frames. |
| Rotate and zoom the camera, then resize the window | Both meshes remain visible and the scene fits the window. |
| Export JSON v1 | A complete JSON file is saved with 31 frames for the sweep and meter-valued measurements. |
| Change a mesh filename to a missing file | Evaluation and export become disabled. Import reports a file error and labels the retained scene. |
| Restore the sample filename and import again | Evaluation and export work again after a successful request. |
| Change pose or sweep inputs after evaluating | The corresponding old result cannot be exported until recalculated. |
| Close and reopen the app | The process exits normally and a fresh window opens. |

Report the failed row, exact input values, visible error text, and whether restarting changes the outcome.
CI covers automated rendering, export bytes, invalid-request recovery, signature checks, and packaged Cocoa launch.
The table above still requires a human check on another Mac; CI success does not establish that it has been completed.
