API_VERSION = '2.1'

import sys
try:
  from ambuild2 import run
  if not run.HasAPI(API_VERSION):
    raise Exception()
except:
  sys.stderr.write('AMBuild {0} must be installed to build this project.\n'.format(API_VERSION))
  sys.stderr.write('http://www.alliedmods.net/ambuild\n')
  sys.exit(1)

prep = run.PrepareBuild(sourcePath=sys.path[0])
prep.default_build_folder = 'obj-' + prep.target_platform
prep.options.add_option('--enable-debug', action='store_const', const='1', dest='debug',
                       help='Enable debugging symbols')
prep.options.add_option('--enable-optimize', action='store_const', const='1', dest='opt',
                       help='Enable optimization')
prep.options.add_option('--metamod', type='string', dest='metamod_path', default='',
                       help='Path to Metamod source code')
prep.options.add_option('--hlsdk', type='string', dest='hlsdk_path', default='',
                       help='Path to the HLSDK')

prep.Configure()