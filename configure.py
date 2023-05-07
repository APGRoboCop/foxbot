# AMBuild Configuration Script for EvoBot, written by Anonymous Player
# vim: set sts=4 ts=8 sw=4 tw=99 et:
API_VERSION = '2.2.3'

import sys
try:
    from ambuild2 import run
    if not run.HasAPI(API_VERSION):
        raise Exception()
except:
    sys.stderr.write('AMBuild {0} must be installed to build this project.\n'.format(API_VERSION))
    sys.stderr.write('http://www.alliedmods.net/ambuild\n')
    sys.exit(1)

def make_objdir_name(p):
    return 'obj-' + util.Platform() + '-' + p.target_arch

builder = run.BuildParser(sourcePath = sys.path[0], api=API_VERSION)
builder.default_arch = 'x86'
builder.default_build_folder = make_objdir_name
# builder.options.add_argument('--hl1sdk', type=str, dest='hl1sdk_path', default=None,
                       # help='Half-Life 1 SDK source tree folder')
# builder.options.add_argument('--mm-path', type=str, dest='mm_path', default=None,
                       # help='Metamod source tree folder')
builder.options.add_argument('--enable-optimize', action='store_const', const='1', dest='optimize',
                       help='Enable optimization')
builder.options.add_argument('--enable-debug', action='store_const', const='1', dest='debug',
                       help='Enable debug')
builder.options.add_argument('--enable-static-lib', action='store_const', const='1', dest='staticlib',
                       help='Enable statically link the sanitizer runtime')
builder.options.add_argument('--enable-shared-lib', action='store_const', const='1', dest='sharedlib',
                       help='Enable dynamically link the sanitizer runtime')
builder.Configure()