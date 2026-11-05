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
    args = parser.parse_args()
    build = args.build_dir.resolve()
    folder = Path(__file__).resolve().parent
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
    # The optional-filter probe intentionally reaches an assertion in Debug.
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    for name in ("index_worktree", "merge_rebase", "refs_remotes", "optional_filter"):
        source = folder / "probes" / (name + ".cpp")
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
        result = subprocess.run([str(exe)], cwd=output, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
        log = result.stdout + "\nprocess_returncode=" + str(result.returncode) + "\n"
        (output / (name + ".log")).write_text(log)
        print("--- " + name + " ---", flush=True)
        print(log, flush=True)
        if name != "optional_filter" and result.returncode:
            raise SystemExit("Unexpected probe failure; see " + str(output))
    print("Saved probe binaries and logs: " + str(output))


if __name__ == "__main__":
    main()
