# Si Windows impide abrir la beta

El mensaje **«Windows no puede acceder al dispositivo, ruta de acceso o archivo especificado»**
aparece antes de que Occlusion Lab inicie. El paquete de CI se abrió desde un ZIP extraído y
pasó la auditoría de DLL; esa prueba no reproduce las reglas de seguridad de cada PC.

1. Confirma que descargaste `occlusion-lab-windows-x64-preview` del
   [run verificado](https://github.com/1816x/Occlusion-Lab/actions/runs/42677129073).
   En la carpeta extraída, `build-commit.txt` debe coincidir con el commit indicado por ese run.
2. En el Explorador, abre **Propiedades → General** del ZIP descargado. Si aparece
   **Desbloquear** en la sección Seguridad, márcalo y extrae el ZIP de nuevo en
   una carpeta nueva. La versión actual se extrae una sola vez. Abre
   `occlusion-desktop.exe` desde esa carpeta. Si no aparece la casilla, pasa al
   siguiente paso.
3. Si continúa el mismo mensaje, mira **Seguridad de Windows → Protección contra
   virus y amenazas → Historial de protección**. Anota si aparece una detección
   para `occlusion-desktop.exe`, su nombre y la acción indicada. También comprueba
   si el ejecutable conserva la opción **Desbloquear** en Propiedades.
4. Si no hay entrada en Historial de protección, abre **Visor de eventos → Registros
   de aplicaciones y servicios → Microsoft → Windows**. Busca una entrada del
   mismo momento y archivo en **AppLocker → EXE and DLL** o en
   **CodeIntegrity → Operational**. Anota el identificador del evento y el motivo.
5. Si tampoco hay entrada allí, abre la carpeta extraída en el Explorador, escribe
   `powershell` en la barra de dirección y pulsa Enter. Ejecuta este comando para
   obtener el código que devuelve Windows al intentar abrir la aplicación:

   ```powershell
   try {
     $app = Start-Process -FilePath (Join-Path (Get-Location) 'occlusion-desktop.exe') -PassThru -ErrorAction Stop
     Start-Sleep -Seconds 5
     $app.Refresh()
     if ($app.HasExited) { "La app se cerró. Código de salida: $($app.ExitCode)" }
     else { "La app sigue en ejecución. PID: $($app.Id)" }
   } catch {
     $problem = $_.Exception
     while ($problem.InnerException) { $problem = $problem.InnerException }
     "Código Win32: $($problem.NativeErrorCode); HRESULT: $($problem.HResult)"
     "Motivo: $($problem.Message)"
   }
   ```

   Si sigue en ejecución pero no ves la ventana, revisa si está minimizada u oculta
   detrás de otra aplicación. Si se cerró, anota el código de salida y busca una
   entrada de la misma hora en **Visor de eventos → Registros de Windows → Aplicación**
   para `occlusion-desktop.exe` (por ejemplo, **Application Error**, evento 1000).
   Comparte el código y el motivo o el módulo con errores; puedes omitir la ruta
   de tu usuario.

Si el equipo pertenece a una organización, su política puede impedir ejecutar
programas sin firma descargados en `Downloads`. El equipo de TI puede confirmar
esa restricción; cambiar el código de la app no concede permisos a Windows.

Para reportar el problema, indica si había **Desbloquear** en el ZIP o en el
`.exe`, si Historial de protección o Visor de eventos muestran una entrada, el
código Win32 o de salida y los primeros caracteres de `build-commit.txt`. No incluyas datos
personales de otros eventos del historial.
