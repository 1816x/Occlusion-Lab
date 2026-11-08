"""Dependency closure checks; the Windows job also launches the extracted package."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    'package_windows', Path(__file__).resolve().parents[1] / 'package-windows-desktop.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class WindowsBundleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.bundle, self.source, self.system = (root / name for name in ('bundle', 'bin', 'system'))
        for folder in (self.bundle, self.source, self.system):
            folder.mkdir()
        (self.bundle / 'app.exe').touch()

    def test_transitive_plugin_dependencies_and_cycles(self):
        (self.bundle / 'platforms').mkdir()
        (self.bundle / 'platforms/qwindows.dll').touch()
        for name in ('engine.dll', 'runtime.dll', 'plugin-dep.dll'):
            (self.source / name).write_text(name)
        (self.system / 'KERNEL32.dll').touch()
        graph = {'app.exe': ['engine.dll'], 'engine.dll': ['runtime.dll'],
                 'runtime.dll': ['engine.dll', 'KERNEL32.dll', 'api-ms-win-core-file-l1-1-0.dll'],
                 'qwindows.dll': ['plugin-dep.dll'], 'plugin-dep.dll': []}
        manifest = module.bundle_dependencies(self.bundle, self.source, self.system,
                                              lambda p: graph[p.name])
        self.assertEqual(len(manifest), 5)
        for name in ('engine.dll', 'runtime.dll', 'plugin-dep.dll'):
            self.assertEqual((self.bundle / name).read_text(), name)
        self.assertFalse((self.bundle / 'KERNEL32.dll').exists())

    def test_missing_transitive_dependency_fails_packaging(self):
        (self.source / 'engine.dll').touch()
        graph = {'app.exe': ['engine.dll'], 'engine.dll': ['missing.dll']}
        with self.assertRaisesRegex(RuntimeError, 'Missing DLL missing.dll required by engine.dll'):
            module.bundle_dependencies(self.bundle, self.source, self.system, lambda p: graph[p.name])


if __name__ == '__main__':
    unittest.main()
