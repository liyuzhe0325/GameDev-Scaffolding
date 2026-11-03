#!/usr/bin/env python3
"""PRE AI contract v1. Standard-library-only scene inspection and transactional edits.

Examples:
  python tools/scene_cli.py inspect scenes/lab.json
  python tools/scene_cli.py set scenes/lab.json light_key position '[2,3,1]'
  python tools/scene_cli.py validate scenes/lab.json
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import os
import re
import sys
import tempfile

ALLOWED_TYPES = {'camera', 'occluder', 'light', 'sprite', 'mesh', 'effect'}
ALLOWED_FIELDS = {'name', 'position'}

def fail(message, code=2):
    print(json.dumps({'ok': False, 'error': {'code': 'PRE_SCENE_ERROR', 'message': str(message)}}, ensure_ascii=False))
    raise SystemExit(code)

def validate(doc):
    if not isinstance(doc, dict) or doc.get('schema_version') != 1:
        raise ValueError('schema_version must be 1')
    render = doc.get('render')
    if not isinstance(render, dict):
        raise ValueError('render must be an object')
    res = render.get('pixel_resolution')
    if not isinstance(res, list) or len(res) != 2 or any(type(n) is not int or n <= 0 or n > 4096 for n in res):
        raise ValueError('pixel_resolution must be two positive integers')
    color = render.get('clear_color')
    if not _vec(color, 4):
        raise ValueError('clear_color must be finite vec4')
    entities = doc.get('entities')
    if not isinstance(entities, list):
        raise ValueError('entities must be an array')
    ids = set()
    for entity in entities:
        if not isinstance(entity, dict) or not all(isinstance(entity.get(k), str) for k in ('id','name','type')):
            raise ValueError('entity needs string id/name/type')
        eid = entity['id']
        if not eid or eid in ids:
            raise ValueError(f'duplicate/empty entity id: {eid}')
        if entity['type'] not in ALLOWED_TYPES:
            raise ValueError(f'unknown entity type: {entity["type"]}')
        if not _vec(entity.get('position'), 3):
            raise ValueError(f'position must be finite vec3: {eid}')
        if 'sprite_image' in entity:
            ref = entity['sprite_image']
            if (entity['type'] != 'sprite' or not isinstance(ref, str) or not ref.startswith('assets/')
                    or not ref.endswith('.png') or len(ref) <= 11
                    or any(ord(c) < 32 or c in '/\\:*?"<>|' for c in ref[7:])):
                raise ValueError('sprite_image must reference assets/<name>.png on a sprite entity')
        if 'script' in entity:
            ref = entity['script']
            if not isinstance(ref, str) or not re.fullmatch(r'scripts/[A-Za-z0-9_-]{1,108}\.lua', ref):
                raise ValueError('script must reference scripts/<name>.lua')
        ids.add(eid)
    if 'tiles' in doc:
        tiles = doc['tiles']
        if not isinstance(tiles, list) or len(tiles) > 8192:
            raise ValueError('tiles must be an array of at most 8192 cells')
        occupied = set()
        for tile in tiles:
            if (not isinstance(tile, dict) or type(tile.get('x')) is not int or type(tile.get('y')) is not int
                    or abs(tile['x']) > 4096 or abs(tile['y']) > 4096
                    or tile.get('kind') not in ('floor', 'wall')):
                raise ValueError('Each tile needs valid integer x/y and a floor or wall kind')
            cell = (tile['x'], tile['y'])
            if cell in occupied:
                raise ValueError('Tile coordinates must be unique')
            occupied.add(cell)
    return True

def _vec(value, n):
    return isinstance(value, list) and len(value) == n and all(type(x) in (int, float) and math.isfinite(x) for x in value)

def read(path):
    data = path.read_bytes()
    doc = json.loads(data)
    validate(doc)
    return doc, hashlib.sha256(data).hexdigest()

def write_atomic(path, doc):
    # No partial writes, with file-system level atomic replacement.
    payload = (json.dumps(doc, indent=2, ensure_ascii=False) + '\n').encode('utf-8')
    fd, name = tempfile.mkstemp(prefix=path.name+'.', suffix='.tmp', dir=str(path.parent))
    try:
        with os.fdopen(fd, 'wb') as out:
            out.write(payload)
            out.flush()
            os.fsync(out.fileno())
        os.replace(name, path)
    finally:
        if os.path.exists(name): os.unlink(name)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    subs = ap.add_subparsers(dest='action', required=True)
    for action in ('inspect', 'validate', 'set'):
        p = subs.add_parser(action)
        p.add_argument('scene', type=Path)
        if action == 'set':
            p.add_argument('entity_id')
            p.add_argument('field', choices=sorted(ALLOWED_FIELDS))
            p.add_argument('value', help='JSON encoded value')
            p.add_argument('--expected-sha256', help='optimistic concurrency guard')
    opts = ap.parse_args()
    try:
        doc, before = read(opts.scene)
        if opts.action == 'inspect':
            print(json.dumps({'ok': True, 'contract_version': 1, 'sha256': before,
                'scene': doc, 'capabilities': {'edit_entity_fields': sorted(ALLOWED_FIELDS),
                'reload_required': True, 'render_implemented': True,
                'render_scope': 'windows_d3d12_sprite_viewport'}}, ensure_ascii=False, indent=2))
        elif opts.action == 'validate':
            print(json.dumps({'ok': True, 'sha256': before, 'entity_count': len(doc['entities'])}))
        else:
            if opts.expected_sha256 and opts.expected_sha256 != before:
                raise ValueError('Scene changed since inspection (sha256 mismatch)')
            target = next((e for e in doc['entities'] if e['id'] == opts.entity_id), None)
            if target is None: raise ValueError(f'Unknown entity: {opts.entity_id}')
            nextval = json.loads(opts.value)
            if opts.field == 'name' and not isinstance(nextval, str): raise ValueError('name must be a string')
            if opts.field == 'position' and not _vec(nextval, 3): raise ValueError('position must be finite vec3')
            target[opts.field] = nextval
            validate(doc)
            # Guard concurrent modifications immediately before replacement (best-effort; not a multi-process lock).
            if read(opts.scene)[1] != before: raise ValueError('Scene changed during edit')
            write_atomic(opts.scene, doc)
            print(json.dumps({'ok': True, 'entity_id': opts.entity_id, 'field': opts.field,
                'new_sha256': read(opts.scene)[1], 'reload_required': True}))
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        fail(str(exc))

if __name__ == '__main__': main()
