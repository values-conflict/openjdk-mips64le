#!/usr/bin/env bash
set -Eeuo pipefail -x

debugLevel='release'
if [ "${1:-}" = '--debug' ]; then
	debugLevel='fastdebug'
	shift
fi

src="${1:?usage: $0 [--debug] <jdk-source-dir>}"

versionConf="$src/make/conf/version-numbers.conf"
if [ ! -f "$versionConf" ]; then
	echo >&2 "error: '$src' does not look like an OpenJDK source tree"
	exit 1
fi

# Find acceptable boot JDK versions and pick the first installed under /opt/java/jdkN.
versions="$(grep '^DEFAULT_ACCEPTABLE_BOOT_VERSIONS=' "$versionConf" | cut -d'"' -f2)"
bootJdk=
for ver in $versions; do
	candidate="/opt/java/jdk$ver"
	if [ -d "$candidate" ]; then
		bootJdk="$candidate"
		break
	fi
done

if [ -z "$bootJdk" ]; then
	echo >&2 "error: no boot JDK found for versions: $versions"
	echo >&2 "error: tried /opt/java/jdk{N} for each N"
	exit 1
fi

echo "using boot JDK: $bootJdk"
"$bootJdk/bin/java" --version

export PATH="/usr/local/bin:$PATH"

cd "$src"

args=(
	# target + toolchain
	--openjdk-target=mips64el-linux-gnuabi64
	--with-boot-jdk="$bootJdk"

	# build identity
	--with-build-user=tianon
	--with-vendor-version-string=Tianon
	--with-vendor-url='https://github.com/values-conflict/openjdk-mips64le'
	--with-vendor-bug-url='https://github.com/values-conflict/openjdk-mips64le'

	# build type
	--with-debug-level="$debugLevel"
	--enable-headless-only

	# bundled libraries (avoid host-library version skew)
	--with-freetype=bundled
	--with-harfbuzz=bundled

	# system library paths
	--with-cups-include=/usr/include
	--with-fontconfig-include=/usr/include
	--with-alsa-include=/usr/include
	--with-alsa-lib=/opt/alsa-stub

	# X11 (configure link test only; /opt/x11-stub built in the Dockerfile)
	--with-x=/usr
	--x-includes=/usr/include
	--x-libraries=/opt/x11-stub

	# cross-compile workarounds
	--disable-warnings-as-errors
	--with-extra-cflags='-I/opt/x11-stub'
	--with-extra-cxxflags='-I/opt/x11-stub'

)

bash ./configure "${args[@]}"

{ time gmake CONF="$debugLevel" images; } 2>&1 \
	| tee "build/linux-mips64el-server-$debugLevel/build.log"
