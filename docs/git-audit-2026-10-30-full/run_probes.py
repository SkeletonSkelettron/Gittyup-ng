#!/usr/bin/env python3
"""Rebuild and run saved audit probes against an existing Linux/Ninja build.

These diagnostic programs demonstrate the behavior described in README.md.
Exit 0 means the probes ran; it does not mean the audited product is correct.
All Git mutations happen in temporary repositories. Artifacts go to /tmp.
"""

import argparse
import os
from pathlib import Path
import resource
import shlex
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[2]
    parser.add_argument("--build-dir", type=Path, default=root / "build")
    names = tuple(sorted(p.stem for p in (Path(__file__).resolve().parent / "probes").glob("*.cpp"))) + ("undo_worktree_detached",)
    parser.add_argument("--only", nargs="+", choices=names)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    folder = Path(__file__).resolve().parent
    import hashlib
    import json
    expected = json.loads((folder / "evidence" / "snapshot.json").read_text())
    current_commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    current_diff = subprocess.check_output(["git", "diff", "HEAD", "--binary"], cwd=root)
    if current_commit != expected["commit"] or hashlib.sha256(current_diff).hexdigest() != expected["diff_sha256"]:
        raise SystemExit("Workspace does not match the saved recheck snapshot; sources are unchanged by this runner.")
    output = Path(tempfile.mkdtemp(prefix="gittyup-audit-probes-"))
    ninja = shutil.which("ninja") or shutil.which("ninja-build")
    if not ninja:
        raise SystemExit("An existing Ninja build and ninja executable are required.")

    commands = subprocess.check_output(
        [ninja, "-C", str(build), "-t", "commands", "test_remotes"], text=True
    ).splitlines()
    compile_args = next(
        shlex.split(line) for line in commands
        if "-c " in line and line.endswith("/test/remotes.cpp")
    )
    # A Ninja link command is surrounded by ': &&' and '&& :'.
    link_line = next(
        line for line in reversed(commands)
        if " -o test/remotes " in line
    )
    link_args = shlex.split(link_line)
    link_args = link_args[2:link_args.index("&&", 2)]
    old_source = compile_args[compile_args.index("-c") + 1]
    old_object = compile_args[compile_args.index("-o") + 1]
    moc_object = "test/CMakeFiles/test_remotes.dir/test_remotes_autogen/mocs_compilation.cpp.o"
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen",
               GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL="/dev/null")
    env.pop("AUDIT_DETACHED", None)
    # Disable core dumps if a reviewed failure path regresses.
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    for name in (args.only or names):
        source = folder / "probes" / (("undo_worktree" if name == "undo_worktree_detached" else name) + ".cpp")
        obj = output / (name + ".o")
        exe = output / name
        compile_probe = []
        i = 0
        while i < len(compile_args):
            token = compile_args[i]
            if token in ("-MT", "-MF"):
                i += 2
                continue
            if token in ("-MD", "-MMD"):
                i += 1
                continue
            compile_probe.append(str(obj) if token == old_object else
                                 str(source) if token == old_source else token)
            i += 1
        compile_probe.append("-I" + str(root / "test"))
        subprocess.run(compile_probe, cwd=build, check=True)

        link_probe = [str(obj) if token == old_object else token for token in link_args
                      if token != moc_object and not token.startswith("-Wl,--dependency-file=")]
        link_probe[link_probe.index("-o") + 1] = str(exe)
        subprocess.run(link_probe, cwd=build, check=True)
        run_env = dict(env, AUDIT_DETACHED="1") if name == "undo_worktree_detached" else env
        result = subprocess.run([str(exe)], cwd=output, env=run_env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
        log = result.stdout + "\nprocess_returncode=" + str(result.returncode) + "\n"
        (output / (name + ".log")).write_text(log)
        print("--- " + name + " ---", flush=True)
        print(log, flush=True)
        if result.returncode:
            raise SystemExit("Unexpected probe failure; see " + str(output))
    print("Saved probe binaries and logs: " + str(output))


if __name__ == "__main__":
    main()
