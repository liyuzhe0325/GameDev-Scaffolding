import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / 'tools' / 'scene_cli.py'
SAMPLE = ROOT / 'scenes' / 'lab.json'

class SceneCLITest(unittest.TestCase):
    def run_cli(self, *args):
        return subprocess.run([sys.executable, str(CLI), *map(str, args)], capture_output=True, text=True)

    def test_sample_is_valid(self):
        r = self.run_cli('validate', SAMPLE)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(json.loads(r.stdout)['entity_count'], 4)

    def test_edit_preserves_other_entities(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'scene.json'
            p.write_bytes(SAMPLE.read_bytes())
            before = json.loads(p.read_text())
            r = self.run_cli('set', p, 'light_key', 'position', '[2,3,1]')
            self.assertEqual(r.returncode, 0, r.stdout)
            after = json.loads(p.read_text())
            self.assertEqual(after['entities'][2]['position'], [2,3,1])
            self.assertEqual(before['entities'][0], after['entities'][0])

    def test_rejects_invalid_position_without_modifying_file(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'scene.json'
            p.write_bytes(SAMPLE.read_bytes()); original = p.read_bytes()
            r = self.run_cli('set', p, 'light_key', 'position', '[1,2]')
            self.assertNotEqual(r.returncode, 0)
            self.assertEqual(p.read_bytes(), original)

    def test_detects_conflicting_revision(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'scene.json'
            p.write_bytes(SAMPLE.read_bytes())
            r = self.run_cli('set', p, 'light_key', 'name', '"Lamp B"', '--expected-sha256', 'bad')
            self.assertNotEqual(r.returncode, 0)
            self.assertEqual(json.loads(p.read_text())['entities'][2]['name'], 'Amber Lamp')

    def test_rejects_gpu_resolution_above_4096(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'scene.json'
            doc = json.loads(SAMPLE.read_text(encoding='utf-8'))
            doc['render']['pixel_resolution'] = [4097, 270]
            p.write_text(json.dumps(doc), encoding='utf-8')
            self.assertNotEqual(self.run_cli('validate', p).returncode, 0)

    def test_sprite_image_reference_is_scene_relative(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'scene.json'
            doc = json.loads(SAMPLE.read_text(encoding='utf-8'))
            sprite = next(entity for entity in doc['entities'] if entity['type'] == 'sprite')
            sprite['sprite_image'] = 'assets/hero.png'
            p.write_text(json.dumps(doc), encoding='utf-8')
            self.assertEqual(self.run_cli('validate', p).returncode, 0)
            sprite['sprite_image'] = 'assets/../hero.png'
            p.write_text(json.dumps(doc), encoding='utf-8')
            self.assertNotEqual(self.run_cli('validate', p).returncode, 0)

    def test_tile_layer_is_preserved_and_duplicate_cells_are_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'scene.json'
            doc = json.loads(SAMPLE.read_text(encoding='utf-8'))
            doc['tiles'] = [{'x': -1, 'y': 2, 'kind': 'floor'}]
            p.write_text(json.dumps(doc), encoding='utf-8')
            self.assertEqual(self.run_cli('set', p, 'hero_01', 'position', '[1,2,0]').returncode, 0)
            self.assertEqual(json.loads(p.read_text(encoding='utf-8'))['tiles'], doc['tiles'])
            doc['tiles'].append({'x': -1, 'y': 2, 'kind': 'wall'})
            p.write_text(json.dumps(doc), encoding='utf-8')
            self.assertNotEqual(self.run_cli('validate', p).returncode, 0)

if __name__ == '__main__': unittest.main()
