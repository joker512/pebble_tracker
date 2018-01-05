import os.path

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

    ctx.env.CXX = 'arm-none-eabi-g++'
    ctx.load('g++')

    ctx.env.CXXFLAGS = list(ctx.env.CFLAGS)
    ctx.env.CXXFLAGS.extend(['-std=c++11', '-Os', '-fPIE', '-fno-unwind-tables', '-fno-exceptions', '-mthumb', '-Wno-write-strings', '-Wno-narrowing'])
    ctx.env.LIB = ['stdc++']

def build(ctx):
    ctx.load('pebble_sdk')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/*.cpp'), target=app_elf)
        binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.pbl_bundle(binaries=binaries, js=ctx.path.ant_glob('src/js/*.js'));
