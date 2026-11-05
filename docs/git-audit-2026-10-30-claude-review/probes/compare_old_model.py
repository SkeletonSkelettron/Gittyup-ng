#!/usr/bin/env python3
"""Compare the current model with an old-model/current-libraries hybrid.

This does NOT build or execute the complete historical project. Both binaries
use the current test libraries and headers. Only the second binary substitutes
DiffModel.cpp from the specified historical ref. All repositories and filter
configuration used by the C++ diagnostic are temporary. The diagnostic may
abort intentionally; exit 0 from this runner only means both runs completed.
"""

import argparse
import json
import os
from pathlib import Path
import resource
import shlex
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--historical-model-ref", default="73fdd1d6")
    args = parser.parse_args()
    folder = Path(__file__).resolve().parent
    root = folder.parents[2]
    build = args.build_dir.resolve() if args.build_dir else root / "build"
    output = Path(tempfile.mkdtemp(prefix="gittyup-required-filter-comparison-"))
    ninja = shutil.which("ninja") or shutil.which("ninja-build")
    if not ninja:
        raise SystemExit("An existing Ninja build is required.")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    commands = subprocess.check_output(
        [ninja, "-C", str(build), "-t", "commands", "test_remotes"], text=True
    ).splitlines()
    compile_args = next(
        shlex.split(line) for line in commands
        if "-c " in line and line.endswith("/test/remotes.cpp")
    )
    link_line = next(
        line for line in reversed(commands) if " -o test/remotes " in line
    )
    link_args = shlex.split(link_line)
    link_args = link_args[2:link_args.index("&&", 2)]
    original_source = compile_args[compile_args.index("-c") + 1]
    original_object = compile_args[compile_args.index("-o") + 1]
    moc_object = "test/CMakeFiles/test_remotes.dir/test_remotes_autogen/mocs_compilation.cpp.o"

    def compile_source(source, obj, extra_include):
        command = []
        i = 0
        while i < len(compile_args):
            token = compile_args[i]
            if token in ("-MT", "-MF"):
                i += 2
                continue
            if token in ("-MD", "-MMD"):
                i += 1
                continue
            command.append(str(obj) if token == original_object else
                           str(source) if token == original_source else token)
            i += 1
        command.append("-I" + str(extra_include))
        subprocess.run(command, cwd=build, check=True)

    probe_obj = output / "required_filter_partial.o"
    compile_source(folder / "required_filter_partial.cpp", probe_obj, root / "test")
    historical_source = output / "DiffModel_historical.cpp"
    historical_source.write_bytes(subprocess.check_output(
        ["git", "show", args.historical_model_ref + ":src/ui/DiffModel.cpp"], cwd=root
    ))
    historical_obj = output / "historical_model.o"
    compile_source(historical_source, historical_obj, root / "src/ui")
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen",
               GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL="/dev/null")
    metadata = {
        "current_commit": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
        "historical_model_commit": subprocess.check_output(
            ["git", "rev-parse", args.historical_model_ref], cwd=root, text=True).strip(),
        "comparison": "Historical DiffModel.cpp linked with current headers and libraries; not a complete historical build",
        "runs": {},
    }
    for label, old_model in (("current", False), ("historical_model_hybrid", True)):
        executable = output / label
        command = [str(probe_obj) if token == original_object else token
                   for token in link_args
                   if token != moc_object and not token.startswith("-Wl,--dependency-file=")]
        if old_model:
            command.insert(command.index(str(probe_obj)) + 1, str(historical_obj))
        command[command.index("-o") + 1] = str(executable)
        subprocess.run(command, cwd=build, check=True)
        result = subprocess.run([str(executable)], cwd=output, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
        log = result.stdout + "\nprocess_returncode=" + str(result.returncode) + "\n"
        (output / (label + ".log")).write_text(log)
        metadata["runs"][label] = {"returncode": result.returncode}
        print(label + ":\n" + log, flush=True)
    (output / "comparison.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print("Saved comparison (hybrid, not complete historical build): " + str(output))


if __name__ == "__main__":
    main()
