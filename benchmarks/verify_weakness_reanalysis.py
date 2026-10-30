"""CLI regression checks using a completed, immutable weakness pilot as fixture.

Usage: python3 benchmarks/verify_weakness_reanalysis.py EXE ARCHIVE TRAIN_RUN VALIDATION_RUN
"""
import concurrent.futures
import copy
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile


def main():
    exe, archive, train, validation = (pathlib.Path(x).resolve() for x in sys.argv[1:])
    roots = json.loads((archive / "train-endpoints.json").read_text())
    heldout = json.loads((archive / "validation-endpoints.json").read_text())
    group = roots["endpoints"][0]["group"]
    with tempfile.TemporaryDirectory() as temporary:
        temporary = pathlib.Path(temporary)

        def check(name, command, message):
            result = subprocess.run([str(exe), *map(str, command)], capture_output=True, text=True)
            assert result.returncode == 2 and message in result.stderr, (name, result.stdout, result.stderr)
            assert not (temporary / (name + "-output")).exists(), name
            return name

        cases = []
        for name in ("turn", "order"):
            bad = copy.deepcopy(roots)
            bad["endpoints"] = bad["endpoints"][:1]
            state = bad["endpoints"][0]["state"]
            if name == "turn":
                state["turn_number"] += 1
            else:
                state["players"] = state["players"][1:] + state["players"][:1]
            fixture = temporary / (name + ".json")
            fixture.write_text(json.dumps(bad))
            cases.append((name, ["--validate", fixture, group], "saved-state SHA256 mismatch"))

        # Only copy the small report; immutable native artifacts are read through symlinks.
        altered = temporary / "wrong-root"
        altered.mkdir()
        for name in ("invocation.json", "continuations.episodes"):
            (altered / name).symlink_to(train / name)
        report = json.loads((train / "result.json").read_text())
        rows = report["episodes"]
        first = next(i for i, row in enumerate(rows) if row["completed"])
        source_root = next(x for x in roots["endpoints"] if x["state_sha256"] == rows[first]["endpoint"])
        other = next(x for x in roots["endpoints"] if x["state_sha256"] != source_root["state_sha256"]
                     and x["state"]["players"] == source_root["state"]["players"])
        rows[first]["endpoint"] = other["state_sha256"]
        (altered / "result.json").write_text(json.dumps(report))
        cases.append(("wrong-root", ["--prepare", archive, altered, validation,
                                    temporary / "wrong-root-output"], "first sample does not match root"))

        crossed = temporary / "split-root"
        crossed.mkdir()
        for name in ("invocation.json", "continuations.episodes"):
            (crossed / name).symlink_to(train / name)
        report = json.loads((train / "result.json").read_text())
        report["episodes"][first]["endpoint"] = heldout["endpoints"][0]["state_sha256"]
        (crossed / "result.json").write_text(json.dumps(report))
        cases.append(("split-root", ["--prepare", archive, crossed, validation,
                                    temporary / "split-root-output"], "continuation root crosses split"))

        bad_archive = temporary / "family-leak"
        bad_archive.mkdir()
        shutil.copy2(archive / "manifest.json", bad_archive)
        shutil.copy2(archive / "train-endpoints.json", bad_archive)
        heldout["endpoints"][0]["source_game_id"] = roots["endpoints"][0]["source_game_id"]
        (bad_archive / "validation-endpoints.json").write_text(json.dumps(heldout))
        cases.append(("family-leak", ["--prepare", bad_archive, train, validation,
                                     temporary / "family-leak-output"], "source family crosses split"))
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            passed = list(pool.map(lambda case: check(*case), cases))
        print("weakness reanalysis: rejected " + ", ".join(passed))


if __name__ == "__main__":
    main()
