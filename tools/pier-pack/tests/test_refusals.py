"""test_refusals.py: the settings and keys a source may not carry.

A flag with no implementation behind it, and a key the builder does not read, are the
same failure: the pack is built, the world is generated, and the terrain is not the one
the author described, with nothing said on either side. Both are refused at build time,
and the two flags are refused again on load, so a pack built by an older tool cannot slip
through. This file holds those refusals still.
"""
import json
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from pierpack import tpl_build, vol_build  # noqa: E402

FIXTURES = os.path.join(HERE, "..", "fixtures")


def _build(mod, src, name):
    path = os.path.join(tempfile.gettempdir(), name + ".json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(src, f)
    return mod.build_file(path, os.path.join(tempfile.gettempdir(), name + ".bin"))


def _load(fixture):
    with open(os.path.join(FIXTURES, fixture), encoding="utf-8") as f:
        return json.load(f)


def test_unimplemented_settings_are_refused():
    for key in ("aquifers_enabled", "ore_veins_enabled"):
        src = _load("islands.json")
        src[key] = True
        try:
            _build(vol_build, src, "refuse_" + key)
        except vol_build.SourceError as e:
            assert key in str(e), e
        else:
            raise AssertionError(key + " was accepted")
        # False is not a refusal: it says the author looked and chose not to.
        src[key] = False
        _build(vol_build, src, "off_" + key)


def test_unknown_keys_are_refused():
    for mod, fixture, name in ((vol_build, "islands.json", "vol"), (tpl_build, "town.json", "tpl")):
        src = _load(fixture)
        # A field Java's format has and this one does not, which is the shape the mistake
        # takes in practice: copied from a datapack and silently dropped.
        src["spawn_target"] = [{"temperature": 0}]
        try:
            _build(mod, src, "unknown_" + name)
        except mod.SourceError as e:
            assert "spawn_target" in str(e), e
        else:
            raise AssertionError(fixture + " accepted an unknown key")


def test_the_fixtures_still_build():
    _build(vol_build, _load("islands.json"), "ok_vol")
    # The template fixture reads a voxel file beside it, so it builds from its own path.
    tpl_build.build_file(os.path.join(FIXTURES, "town.json"),
                         os.path.join(tempfile.gettempdir(), "ok_tpl.bin"))


if __name__ == "__main__":
    test_unimplemented_settings_are_refused()
    test_unknown_keys_are_refused()
    test_the_fixtures_still_build()
    print("test_refusals: ok")
