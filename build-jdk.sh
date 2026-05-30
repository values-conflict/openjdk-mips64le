#!/usr/bin/env bash
set -Eeuo pipefail -x

src="${1:?usage: $0 <jdk-source-dir>}"

versionConf="$src/make/conf/version-numbers.conf"
if [ ! -f "$versionConf" ]; then
	echo >&2 "error: '$src' does not look like an OpenJDK source tree"
	exit 1
fi

# find acceptable boot JDK versions and pick the first one installed under /opt/java/jdkN
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

bash ./configure \
	--openjdk-target=mips64el-linux-gnuabi64 \
	--with-boot-jdk="$bootJdk" \
	--with-debug-level=release \
	--enable-headless-only \
	--with-freetype=bundled \
	--with-harfbuzz=bundled \
	--with-cups-include=/usr/include \
	--with-fontconfig-include=/usr/include \
	--with-alsa-include=/usr/include \
	--with-alsa-lib=/opt/alsa-stub \
	--disable-warnings-as-errors \
	--with-extra-cflags='-I/usr/include' \
	--with-extra-cxxflags='-I/usr/include' \
	--with-vendor-version-string=Tianon \
	--with-vendor-url='https://github.com/values-conflict/openjdk-mips64le' \
	--with-vendor-bug-url='https://github.com/values-conflict/openjdk-mips64le'

{ time gmake CONF=release images; } \
	2>&1 | tee build/linux-mips64el-server-release/build.log
