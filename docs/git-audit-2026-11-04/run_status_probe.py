#!/usr/bin/env python3
# Linux/Ninja helper; compile current sources first. Writes only audit logs and /tmp.
import os, pathlib, shlex, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[2]
build = root/'build'
source = root/'docs/git-audit-2026-11-04/probes/status_pending.cpp'
output = pathlib.Path(tempfile.mkdtemp(prefix='gittyup-status-review-'))
commands = subprocess.check_output(['ninja', '-C', str(build), '-t', 'commands', 'test_remotes'], text=True).splitlines()
compile_args = next(shlex.split(l) for l in commands if '-c ' in l and l.endswith('/test/remotes.cpp'))
link_args = shlex.split(next(l for l in reversed(commands) if ' -o test/remotes ' in l))
link_args = link_args[2:link_args.index('&&', 2)]
old_source=compile_args[compile_args.index('-c')+1]
old_object=compile_args[compile_args.index('-o')+1]
obj=output/'status_pending.o'; exe=output/'status_pending'
a=[]; i=0
while i<len(compile_args):
 t=compile_args[i]
 if t in ('-MT','-MF'): i+=2; continue
 if t in ('-MD','-MMD'): i+=1; continue
 a.append(str(obj) if t==old_object else str(source) if t==old_source else t)
 i+=1
a.append('-I'+str(root/'test'))
subprocess.run(a,cwd=build,check=True)
a=[str(obj) if t==old_object else t for t in link_args if t!='test/CMakeFiles/test_remotes.dir/test_remotes_autogen/mocs_compilation.cpp.o' and not t.startswith('-Wl,--dependency-file=')]
a[a.index('-o')+1]=str(exe)
subprocess.run(a,cwd=build,check=True)
env=dict(os.environ,QT_QPA_PLATFORM='offscreen',GIT_CONFIG_NOSYSTEM='1',GIT_CONFIG_GLOBAL='/dev/null')
r=subprocess.run([str(exe)],cwd=output,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=45)
log=r.stdout+'\nprocess_returncode='+str(r.returncode)+'\n'
(root/'docs/git-audit-2026-11-04/evidence/status_pending.log').write_text(log)
print(log);print(output)
