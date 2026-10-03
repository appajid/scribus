#!/usr/bin/env bash
# Prepare an unsigned, self-contained macOS ARM64 test app and ZIP.
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 /path/to/Apscribe.app /path/to/output-directory" >&2
  exit 2
fi

source_app=$1
output_dir=$2
staged_app="$output_dir/Apscribe.app"
archive="$output_dir/Apscribe-2.0.0-rc1-macos-arm64-unsigned-test.zip"
contents="$staged_app/Contents"
frameworks="$contents/Frameworks"
python_version=3.14
python_framework="$frameworks/Python.framework"
python_home="$python_framework/Versions/$python_version"

[[ -d "$source_app/Contents" ]] || { echo "Missing source app: $source_app" >&2; exit 1; }
[[ ! -e "$staged_app" && ! -e "$archive" ]] || {
  echo "Output app or ZIP already exists; use a new output directory" >&2
  exit 1
}
mkdir -p "$output_dir"
ditto "$source_app" "$staged_app"

# macdeployqt handles Qt and ordinary Homebrew dylibs. It may report missing
# optional QtPdf/virtual-keyboard modules from the build machine; those two
# plugins are deliberately omitted below because Apscribe does not use them.
macdeployqt "$staged_app" -verbose=1
[[ -f "$frameworks/QtCore.framework/Versions/A/QtCore" ]]
[[ -f "$contents/PlugIns/platforms/libqcocoa.dylib" ]]

quarantine="$output_dir/optional-qt-plugins"
mkdir -p "$quarantine"
for plugin in \
  "$contents/PlugIns/imageformats/libqpdf.dylib" \
  "$contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib"; do
  if [[ -f "$plugin" ]]; then
    mv "$plugin" "$quarantine/$(basename "$plugin")"
  fi
done

python_source="$(realpath "$(brew --prefix python@$python_version)/Frameworks/Python.framework")"
ditto "$python_source" "$python_framework"
install_name_tool -id \
  "@executable_path/../Frameworks/Python.framework/Versions/$python_version/Python" \
  "$python_home/Python"
site_packages="$python_home/lib/python$python_version/site-packages"
if [[ -L "$site_packages" ]]; then
  unlink "$site_packages"
  mkdir "$site_packages"
fi

# Also carry the offscreen platform plugin for packaged headless smoke tests.
offscreen_source="$(qtpaths6 --plugin-dir)/platforms/libqoffscreen.dylib"
offscreen_target="$contents/PlugIns/platforms/libqoffscreen.dylib"
ditto "$offscreen_source" "$offscreen_target"
install_name_tool -change @rpath/QtGui.framework/Versions/A/QtGui \
  @executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui "$offscreen_target"
install_name_tool -change @rpath/QtCore.framework/Versions/A/QtCore \
  @executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore "$offscreen_target"

# Rewrite Homebrew dependencies in Scripter and Python's extension modules.
# Iterate because a newly copied library can itself depend on another dylib.
for pass in 1 2 3 4; do
  rewrites=0
  while IFS= read -r -d '' binary; do
    while IFS= read -r dependency; do
      [[ -n "$dependency" ]] || continue
      if [[ "$(basename "$dependency")" == Python ]]; then
        replacement="@executable_path/../Frameworks/Python.framework/Versions/$python_version/Python"
      else
        library_name="$(basename "$dependency")"
        replacement="@executable_path/../Frameworks/$library_name"
        if [[ ! -e "$frameworks/$library_name" ]]; then
          ditto "$(realpath "$dependency")" "$frameworks/$library_name"
          install_name_tool -id "$replacement" "$frameworks/$library_name"
        fi
      fi
      install_name_tool -change "$dependency" "$replacement" "$binary"
      rewrites=$((rewrites + 1))
    done < <(otool -L "$binary" 2>/dev/null | awk '/^[[:space:]]*\/opt\/homebrew\// {print $1}')

    # macdeployqt sometimes copies an indirect Homebrew library without its
    # own @rpath dependencies (for example JPEG XL -> libjxl_cms). Resolve
    # those from Homebrew's linked lib directory into the same bundle.
    while IFS= read -r dependency; do
      library_name="$(basename "$dependency")"
      source_library="/opt/homebrew/lib/$library_name"
      replacement="@executable_path/../Frameworks/$library_name"
      if [[ ! -e "$frameworks/$library_name" ]]; then
        [[ -e "$source_library" ]] || continue
        ditto "$(realpath "$source_library")" "$frameworks/$library_name"
        install_name_tool -id "$replacement" "$frameworks/$library_name"
      fi
      install_name_tool -change "$dependency" "$replacement" "$binary"
      rewrites=$((rewrites + 1))
    done < <(otool -L "$binary" 2>/dev/null | \
      awk 'index($1, "@rpath/lib") == 1 && $1 ~ /\.dylib$/ {print $1}')
  done < <(find "$contents" -type f \
    \( -name '*.dylib' -o -name '*.so' -o -name Apscribe -o -name Python \) -print0)
  [[ "$rewrites" -gt 0 ]] || break
done

dependency_dump="$(find "$contents" -type f \
  \( -name '*.dylib' -o -name '*.so' -o -name Apscribe -o -name Python \) \
  -print0 | xargs -0 -n1 otool -L 2>/dev/null)"
if grep -q '^[[:space:]]*/opt/homebrew/' <<< "$dependency_dump"; then
  echo 'A Homebrew dynamic-library dependency remains in the bundle' >&2
  exit 1
fi
if awk 'index($1, "@rpath/lib") == 1 && $1 ~ /\.dylib$/ {print $1}' \
  <<< "$dependency_dump" | while read -r dependency; do
    [[ -e "$frameworks/$(basename "$dependency")" ]] || exit 1
  done; then
  :
else
  echo 'An unbundled @rpath dynamic-library dependency remains' >&2
  exit 1
fi

# install_name_tool invalidates signatures. Sign nested Python modules first,
# then the enclosing app, and verify the result before distributing for test.
find "$contents" -type f \
  \( -name '*.dylib' -o -name '*.so' -o -name Python \) -print0 |
  xargs -0 -n1 codesign --force --sign - >/dev/null 2>&1
codesign --force --deep --sign - "$staged_app"
codesign --verify --deep --strict "$staged_app"

export QT_QPA_PLATFORM=offscreen
export PYTHONDONTWRITEBYTECODE=1
"$contents/MacOS/Apscribe" --version | grep -F 'Apscribe Version 2.0.0'
smoke_result="$(mktemp /private/tmp/apscribe-scripter-smoke.XXXXXX)"
export APSCRIBE_EXPECTED_PYTHON_PREFIX="$python_home"
export APSCRIBE_SMOKE_RESULT="$smoke_result"
"$contents/MacOS/Apscribe" --no-gui --python-script \
  "$(dirname "$0")/macos-scripter-smoke.py"
[[ -s "$smoke_result" ]] || { echo 'Bundled Scripter smoke failed' >&2; exit 1; }
codesign --verify --deep --strict "$staged_app"

ditto -c -k --sequesterRsrc --keepParent "$staged_app" "$archive"
(cd "$output_dir" && shasum -a 256 "$(basename "$archive")" \
  > "$(basename "$archive").sha256")
echo "Candidate Mac ZIP: $archive"
