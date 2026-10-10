"""Check standalone AXS stm_lcd dependency resolution without hardware access."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

LCD_PIN = 'c359e54a657be38aec90c797ea19ee3d492d9284'
COMPONENT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lcd-source', required=True, type=Path)
    parser.add_argument('--common-source', required=True, type=Path)
    parser.add_argument('--lcd-repository', help='Git mirror; defaults to the local --lcd-source checkout')
    parser.add_argument('--peer-source', type=Path)
    parser.add_argument('--build-dir', type=Path, default=COMPONENT / 'build/dependency-tests')
    parser.add_argument('--c-compiler', default='gcc')
    parser.add_argument('--cxx-compiler', default='g++')
    args = parser.parse_args()
    if args.lcd_repository is None:
        args.lcd_repository = args.lcd_source.resolve().as_posix()
    if Path(args.lcd_repository).exists():
        args.lcd_repository = Path(args.lcd_repository).resolve().as_posix()
    output = args.build_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sandbox = Path(tempfile.mkdtemp(prefix='dependency-', dir=output))
    lcd = args.lcd_source.resolve()
    common = args.common_source.resolve()
    peer = args.peer_source.resolve() if args.peer_source else None
    assert (lcd / 'CMakeLists.txt').is_file() and (common / 'CMakeLists.txt').is_file()
    modes = ['target', 'alias', 'sibling', 'explicit', 'override', 'explicit_precedence',
             'override_precedence', 'target_precedence', 'fetch', 'missing_off',
             'invalid_explicit', 'invalid_override', 'wrong_target', 'disconnected']
    if peer:
        modes += ['dual_forward', 'dual_reverse']
    for mode in modes:
        source = sandbox / mode
        source.mkdir()
        component = source / 'lib' / COMPONENT.name
        shutil.copytree(COMPONENT, component, ignore=shutil.ignore_patterns('.git', 'build', '__pycache__'))
        prelude = f'add_subdirectory("{common.as_posix()}" common)\n'
        flags = ['-DSTM_COMMON_FETCH=OFF', '-DSTM_LCD_FETCH=OFF',
                 f'-DSTM_LCD_GIT_REPOSITORY={(source / "missing-repository").as_posix()}']
        expected = lcd
        error = None
        if mode in ['sibling', 'explicit_precedence', 'override_precedence']:
            sibling = source / 'lib/stm_lcd'
            shutil.copytree(lcd, sibling, ignore=shutil.ignore_patterns('.git', 'build', '__pycache__'))
            if mode == 'sibling':
                expected = sibling
        if mode in ['target', 'target_precedence']:
            prelude += f'add_subdirectory("{lcd.as_posix()}" lcd)\n'
            flags += [f'-DSTM_LCD_SOURCE_DIR={(source / "invalid").as_posix()}']
        elif mode == 'alias':
            prelude += 'add_library(lcd_provider INTERFACE)\nadd_library(stm_lcd ALIAS lcd_provider)\n'
            prelude += f'target_include_directories(lcd_provider INTERFACE "{(lcd / "include").as_posix()}")\n'
        elif mode in ['explicit', 'explicit_precedence']:
            flags += [f'-DSTM_LCD_SOURCE_DIR={lcd.as_posix()}',
                      f'-DFETCHCONTENT_SOURCE_DIR_STM_LCD={(source / "invalid").as_posix()}']
        elif mode in ['override', 'override_precedence']:
            flags += [f'-DFETCHCONTENT_SOURCE_DIR_STM_LCD={lcd.as_posix()}']
        elif mode in ['fetch', 'dual_forward', 'dual_reverse']:
            flags += ['-DSTM_LCD_FETCH=ON', f'-DSTM_LCD_GIT_REPOSITORY={args.lcd_repository}']
        elif mode == 'missing_off':
            error = 'stm_lcd is missing'
        elif mode in ['invalid_explicit', 'invalid_override']:
            key = 'STM_LCD_SOURCE_DIR' if mode == 'invalid_explicit' else 'FETCHCONTENT_SOURCE_DIR_STM_LCD'
            flags += [f'-D{key}={(source / "invalid").as_posix()}', '-DSTM_LCD_FETCH=ON']
            error = 'stm_lcd local source is invalid'
        elif mode == 'wrong_target':
            wrong = source / 'wrong'
            wrong.mkdir()
            (wrong / 'CMakeLists.txt').write_text('add_library(not_stm_lcd INTERFACE)\n', encoding='utf-8')
            flags += [f'-DSTM_LCD_SOURCE_DIR={wrong.as_posix()}']
            error = 'stm_lcd dependency did not provide'
        elif mode == 'disconnected':
            flags += ['-DSTM_LCD_FETCH=ON', '-DFETCHCONTENT_FULLY_DISCONNECTED=ON']
            error = 'stm_lcd is missing in fully disconnected mode'
        order = [COMPONENT.name]
        if mode.startswith('dual_'):
            shutil.copytree(peer, source / 'lib' / peer.name,
                            ignore=shutil.ignore_patterns('.git', 'build', '__pycache__'))
            order.append(peer.name)
            if mode == 'dual_reverse':
                order.reverse()
        cmake = 'cmake_minimum_required(VERSION 3.22)\nproject(axs_dependencies LANGUAGES C CXX)\n'
        cmake += 'add_compile_options(-Wall -Wextra -Werror)\n' + prelude
        cmake += ''.join(f'add_subdirectory(lib/{name})\n' for name in order)
        cmake += 'get_target_property(lcd_source stm_lcd SOURCE_DIR)\nfile(WRITE "${CMAKE_BINARY_DIR}/lcd-source.txt" "${lcd_source}")\n'
        headers = ''.join(f'#include "{name}.h"\n' for name in order)
        (source / 'headers.c').write_text(headers, encoding='utf-8')
        (source / 'headers.cpp').write_text(headers, encoding='utf-8')
        cmake += 'add_library(consumers OBJECT headers.c headers.cpp)\ntarget_compile_features(consumers PRIVATE c_std_11 cxx_std_17)\n'
        cmake += 'target_link_libraries(consumers PRIVATE ' + ' '.join(order) + ')\n'
        (source / 'CMakeLists.txt').write_text(cmake, encoding='utf-8')
        binary = source / 'out'
        result = subprocess.run(['cmake', '-S', str(source), '-B', str(binary), '-G', 'Ninja',
                                 f'-DCMAKE_C_COMPILER={args.c_compiler}',
                                 f'-DCMAKE_CXX_COMPILER={args.cxx_compiler}', *flags],
                                capture_output=True, text=True, encoding='utf-8', errors='replace')
        log = result.stdout + result.stderr
        (output / f'{mode}-configure.log').write_text(log, encoding='utf-8')
        if error:
            assert result.returncode and error in log, (mode, log[-3000:])
            assert not (binary / '_deps/stm_lcd-subbuild').exists(), mode
        else:
            assert result.returncode == 0, (mode, log[-3000:])
            actual = Path((binary / 'lcd-source.txt').read_text()).resolve()
            if mode in ['fetch', 'dual_forward', 'dual_reverse']:
                sha = subprocess.check_output(['git', '-C', str(actual), 'rev-parse', 'HEAD'], text=True).strip()
                assert sha == LCD_PIN, (mode, sha)
                assert actual.is_relative_to(binary), (mode, actual)
                assert len(list((binary / '_deps').glob('stm_lcd-src'))) == 1
            elif mode != 'alias':
                assert actual == expected.resolve(), (mode, actual, expected)
                assert not (binary / '_deps/stm_lcd-subbuild').exists(), mode
            result = subprocess.run(['cmake', '--build', str(binary)], capture_output=True,
                                    text=True, encoding='utf-8', errors='replace')
            (output / f'{mode}-build.log').write_text(result.stdout + result.stderr, encoding='utf-8')
            assert result.returncode == 0, (mode, result.stderr, result.stdout[-3000:])
        print(f'PASS: {COMPONENT.name} dependency {mode}', flush=True)
    print(f'PASS: {len(modes)} dependency cases; C11/C++17; no hardware testing', flush=True)


if __name__ == '__main__':
    main()
