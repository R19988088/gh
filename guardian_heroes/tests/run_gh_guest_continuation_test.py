#!/usr/bin/env python3
"""Compile original and PC-dispatch paths, then resume serialized guest frames."""
from pathlib import Path
import importlib.util
import re
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
project = Path(__file__).resolve().parents[1]
tool = project / 'tools/gh_guest_continuation_codegen.py'
spec = importlib.util.spec_from_file_location('continuation_codegen', tool)
codegen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codegen)
source = (project / 'tests/gh_guest_continuation_fixture.cpp').read_text()
real_source = (project / 'generated/default/guardian_heroes_recomp.0.cpp').read_text()
leaf = re.search(r'DEFINE_REX_FUNC\(sub_82421C70\) \{.*?\n\}', real_source, re.S).group(0)
source += '\n'+leaf+'\n'
real_tail_source = (project / 'generated/default/guardian_heroes_recomp.3.cpp').read_text()
tail = re.search(r'DEFINE_REX_FUNC\(sub_82410108\) \{.*?\n\}', real_tail_source, re.S).group(0)
source += '\n'+tail+'\n'
# Unsupported operations must fail offline, before emitting executable output.
for unsupported in [
    'DEFINE_REX_FUNC(sub_1000) {\nPPCRegister persistent{};\n// blr\nreturn;\n}',
    'DEFINE_REX_FUNC(sub_1000) {\n// bl 0x2000\nppc_setjmp(ctx.r3.u32);\n}',
    'DEFINE_REX_FUNC(sub_1000) {\n// bl 0x2000\nNtWait(ctx, base);\n}',
    'DEFINE_REX_FUNC(sub_1000) {\n// blr\ntry { Unknown(); } catch (...) {}\n}',
    'DEFINE_REX_FUNC(sub_1000) {\n// b 0x2000\ngoto loc_2000;\n}',
]:
    try:
        codegen.transform(unsupported, 'gh_guest_continuation_support.h')
    except codegen.Unsupported:
        pass
    else:
        raise AssertionError('unsupported continuation was accepted')
with tempfile.TemporaryDirectory(prefix='gh-continuation-') as temporary:
    temporary = Path(temporary)
    (temporary / 'original_continuation_fixture.h').write_text(source)
    generated = temporary / 'continuation.cpp'
    generated.write_text(codegen.transform(source, 'gh_guest_continuation_support.h'))
    binary = temporary / 'continuation-test'
    subprocess.run(['clang++', '-std=c++23', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(project / 'src'), '-I'+str(project / 'tests'),
                    '-I'+str(temporary), str(project / 'src/gh_guest_continuation.cpp'),
                    str(project / 'src/gh_guest_continuation_title.cpp'),
                    str(generated), str(project / 'tests/gh_guest_continuation_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
