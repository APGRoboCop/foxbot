#
# YaPB - Counter-Strike Bot based on PODBot by Markus Klinge.
# Copyright © 2004-2020 YaPB Development Team <team@yapb.ru>.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

project (
    'foxbot',
    'cpp',
    
    version: '0.793',
    license: 'GPL',
    
    default_options: [
        'buildtype=release',
        'b_ndebug=if-release',
        'cpp_std=c++14',
        'warning_level=1',
        'werror=false',
        'backend=ninja',
        'strip=true',
        'optimization=3',
        'default_library=static',
        'cpp_eh=none'
    ],
    meson_version: '>=0.48.0')

buildCompiler = meson.get_compiler ('cpp')
buildSystem = host_machine.system ()
buildVersion = meson.project_version ()

compilerId = buildCompiler.get_id ()
compilerVersion = buildCompiler.version ()

isOptimize = get_option ('buildtype') == 'release'
isVC = compilerId == 'msvc' or compilerId == 'intel-cl' or compilerId == 'clang-cl'
isGCC = compilerId == 'gcc'
isIntel = compilerId == 'intel' or compilerId == 'intel-cl'
isCLang = compilerId == 'clang'
isWindows = buildSystem == 'windows'
isLinux = buildSystem == 'linux'
isDarwin = buildSystem == 'darwin'

flagsLinker = []
flagsCompiler = []

if isLinux
   flagsCompiler += '-Dlinux'
endif

if isCLang or isGCC or (isIntel and not isWindows)
    flagsCompiler += [
        '-m32',
        '-fno-threadsafe-statics',
        '-fno-exceptions',
        '-fno-rtti',
        '-Wno-write-strings',
    ]
    
    if isOptimize
        flagsCompiler += '-msse2'
    endif
    
    if isLinux
        flagsLinker += [
            '-m32',
            '-lm',
            '-ldl'
        ]
    endif
endif

if isIntel and (isLinux or isDarwin)
    flagsLinker += [
        '-static-intel',
        '-no-intel-extensions'
    ]
endif

if isLinux or isDarwin
    if isDarwin
        flagsCompiler += '-mmacosx-version-min=10.9'
        flagsLinker += [
            '-dynamiclib',
            '-lstdc++',
            '-mmacosx-version-min=10.9'
        ]
    else
        flagsLinker += '-static-libgcc'
    endif
    
    if not isOptimize
        flagsCompiler += [
            '-g3',
            '-ggdb',
            '-O3',
            '-DCR_DEBUG'
        ]
    else
        flagsCompiler += [
            '-mtune=generic',
            '-msse2',
            '-mfpmath=sse',
            '-fno-builtin',
            '-funroll-loops',
            '-fomit-frame-pointer',
            '-fno-stack-protector',
            '-fvisibility=hidden',
            '-fvisibility-inlines-hidden'
        ]
        
        if isIntel 
            flagsCompiler += [
                '-funroll-all-loops',
                '-ipo',
                '-wd11076', 
                '-wd11074'
            ]
            
            flagsLinker += [
                '-cxxlib-nostd',
                '-Wl,--no-undefined,-z,notext,--gc-sections',
                '-ipo'
            ]
        elif isCLang and not isDarwin
             llvmLinker = find_program ('lld', required: false)
             
            if llvmLinker.found() == true
                flagsLinker += '-fuse-ld=' + llvmLinker.path ().split ('/')[-1]
            endif
            
            flagsLinker += [
                '-nostdlib++',
                '-Wunused-command-line-argument',
                '-Wl,-z,notext',
                '--no-undefined'
            ]
        elif isGCC and not isDarwin
            flagsCompiler += '-funroll-all-loops'
            flagsLinker += '-Wl,--no-undefined'
        endif
    endif
endif

if isWindows and (isVC or isIntel)
    flagsLinker += [
        '/MACHINE:X86',
        'user32.lib',
        'ws2_32.lib'
    ]
    
    flagsCompiler += [
        '/TP',
        '/Zc:strictStrings-'
    ]
    
    if isOptimize
        flagsCompiler += '/GL'
        flagsLinker += '/LTCG'
    endif
    
elif isWindows and (isCLang or isGCC)
    if isCLang
        flagsLinker += '-Wl,/MACHINE:X86'
    else
        flagsLinker += [
            '-static-libgcc',
            '-Wl,--add-stdcall-alias'
        ]
    endif
    
    flagsLinker += [
        '-luser32',
        '-lws2_32'
    ]
endif

add_global_arguments (flagsCompiler, language: 'cpp')
add_global_link_arguments (flagsLinker, language: 'cpp')

sourceFiles = files (
   'bot.cpp',
   'bot_client.cpp',
   'bot_combat.cpp',
   'bot_job_assessors.cpp',
   'bot_job_functions.cpp',
   'bot_job_think.cpp',
   'bot_navigate.cpp',
   'bot_start.cpp',
   'botcam.cpp',
   'dll.cpp',
   'engine.cpp',
   'h_export.cpp',
   'linkfunc.cpp',
   'meta_api.cpp',
   'sdk_util.cpp',
   'util.cpp',
   'version.cpp',
   'waypoint.cpp'
)

sdkIncludes = [
    join_paths('hlsdk', 'common'),
    join_paths('hlsdk', 'dlls'),
    join_paths('hlsdk', 'engine'),
    join_paths('hlsdk', 'pm_shared'),
]

includes = include_directories (sdkIncludes, ['.', 'metamod'], is_system: true)

shared_library (
    'foxbot', 
    sourceFiles,
    include_directories: includes,
    
    gnu_symbol_visibility: 'hidden',
    name_prefix: '')
