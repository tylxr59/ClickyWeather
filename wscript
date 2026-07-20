import json
import re

#
# This file is the default set of rules to compile a Pebble application.
#
# Feel free to customize this to your needs.
#
top = '.'
out = 'build'


def _c_escape(value):
    return (value.replace('\\', '\\\\')
                 .replace('"', '\\"')
                 .replace('\n', '\\n')
                 .replace('\r', '')
                 .replace('\t', ' '))


def generate_version_header(ctx):
    """Generate the on-watch release notes from CHANGELOG.md."""
    changelog = ctx.path.find_node('CHANGELOG.md')
    if not changelog:
        ctx.fatal('CHANGELOG.md is required for release notes')

    text = changelog.read()
    match = re.search(
        r'(?m)^##\s+(\d+)\.(\d+)\.(\d+)[^\n]*\n(.*?)(?=^##\s|\Z)',
        text, re.S)
    if not match:
        ctx.fatal('CHANGELOG.md has no semantic-version release entry')

    major, minor, patch = [int(value) for value in match.group(1, 2, 3)]
    label = '%d.%d.%d' % (major, minor, patch)
    bullets = []
    for line in match.group(4).splitlines():
        line = line.strip()
        if line.startswith('-'):
            bullets.append(line[1:].strip())
    if not bullets:
        ctx.fatal('CHANGELOG.md %s has no release-note bullets' % label)

    versions = re.findall(r'(?m)^##\s+(\d+)\.(\d+)\.(\d+)', text)
    if len(versions) >= 2:
        newest = tuple(int(value) for value in versions[0])
        previous = tuple(int(value) for value in versions[1])
        if newest <= previous:
            ctx.fatal('CHANGELOG.md versions must be newest-first')

    package = ctx.path.find_node('package.json')
    package_version = json.loads(package.read()).get('version', '')
    if package_version != label:
        ctx.fatal('package.json version (%s) must match CHANGELOG.md (%s)'
                  % (package_version, label))

    code = major * 10000 + minor * 100 + patch
    ctx.path.make_node('src/c/version_gen.h').write(
        '#pragma once\n'
        '// Generated from CHANGELOG.md by wscript. Do not edit.\n'
        '#define APP_VERSION_CODE %d\n' % code +
        '#define APP_VERSION_LABEL "%s"\n' % _c_escape(label) +
        '#define APP_UPDATE_NOTES "%s"\n' % _c_escape('\n'.join(bullets)))


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    """
    This method is used to configure your build. ctx.load(`pebble_sdk`) automatically configures
    a build for each valid platform in `targetPlatforms`. Platform-specific configuration: add your
    change after calling ctx.load('pebble_sdk') and make sure to set the correct environment first.
    Universal configuration: add your change prior to calling ctx.load('pebble_sdk').
    """
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')
    generate_version_header(ctx)

    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'), target=app_elf, bin_type='app')
        binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json',
                                         'src/common/**/*.js']),
                   js_entry_file='src/pkjs/index.js')
