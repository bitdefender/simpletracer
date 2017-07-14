#!/usr/bin/env bash

[ $# -ne 2 ] && { echo "Usage: $0 <branch-name> <release-version>" >&2; exit 1; }
CWD=`pwd`
SD="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
SRC_DIR=$CWD
echo "Checking requirements ..."
[ "$GITHUB_API_TOKEN" = "" ] && { echo "Error: Set GITHUB_API_TOKEN accordingly"; exit 1; }

echo "Updating the repository ..."
(cd $SRC_DIR && git pull https://$GITHUB_API_TOKEN@github.com/teodor-stoenescu/simpletracer.git $1)

echo "Cleaning the build environment ..."
BUILD_DIR="$CWD/build-river-tools"
[ "$BUILD_DIR" = "/" ] && { echo "Error: Interesting attempt to wipe the disk"; exit 1; }
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

echo "Rebuilding the solution ..."
(cd $BUILD_DIR && cmake $SRC_DIR -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DRIVER_SDK_VERSION="$2" && cmake --build .)

echo "Setting symlinks for core libraries ..."

LIBC_PATH=$(find /lib -name libc.so.6 -path *i386*)
if [ "$LIBC_PATH" == "" ]; then
  echo "libc.so.6 not found. Exiting."
  exit 1
fi

ln -s -T $LIBC_PATH $BUILD_DIR/lib/libc.so

LIBPTHREAD_PATH=$(find /lib -name libpthread.so.0 -path *i386*)
if [ "$LIBPTHREAD_PATH" == "" ]; then
  echo "libpthread.so.0 not found. Exiting."
  exit 1
fi

ln -s -T $LIBPTHREAD_PATH $BUILD_DIR/lib/libpthread.so

echo "Downloading the corpus ..." 
for corpus in "libxml2" "http-parser"; do
	CORPUS_DIR="$CWD/corpus-$corpus"
	if [ -d $CORPUS_DIR ]; then
		continue
	fi
	[ "$CORPUS_DIR" = "/" ] && { echo "Error: Interesting attempt to wipe the disk"; exit 1; }
	rm -rf $CORPUS_DIR
	mkdir $CORPUS_DIR
	$SD/libs.sh teodor-stoenescu simpletracer "$2" "corpus-$corpus.zip" "$CORPUS_DIR/corpus-$corpus.zip"
	unzip "$CORPUS_DIR/corpus-$corpus.zip" -d $CORPUS_DIR
	rm "$CORPUS_DIR/corpus-$corpus.zip"
done
