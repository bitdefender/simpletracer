#!/usr/bin/env bash

[ "$1" != "" ] || [ "$2" != "" ] || { echo "Usage: $0 <branch-name>" >&2; exit 1; }
CWD="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
SRC_DIR="$CWD/simpletracer"
echo "Updating the repository ..."
(cd $SRC_DIR && git pull origin $1)

echo "Checking requirements ..."
[ "$GITHUB_API_TOKEN" = "" ] && { echo "Error: Set GITHUB_API_TOKEN accordingly"; exit 1; }

echo "Cleaning the build environment ..."
BUILD_DIR="$CWD/build"
[ "$BUILD_DIR" = "/" ] && { echo "Error: Interesting attempt to wipe the disk"; exit 1; }
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

echo "Rebuilding the solution ..."
(cd $BUILD_DIR && cmake $SRC_DIR -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DRIVER_SDK_VERSION="$2" && cmake --build .)

echo "Downloading the corpus ..." 
for corpus in "libxml2" "http-parser"; do
	CORPUS_DIR="$CWD/corpus-$corpus"
	if [ -d $CORPUS_DIR ]; then
		continue
	fi
	[ "$CORPUS_DIR" = "/" ] && { echo "Error: Interesting attempt to wipe the disk"; exit 1; }
	rm -rf $CORPUS_DIR
	mkdir $CORPUS_DIR
	$CWD/simpletracer/libs.sh teodor-stoenescu simpletracer "$2" "corpus-$corpus.zip" "$CORPUS_DIR/corpus-$corpus.zip"
	unzip "$CORPUS_DIR/corpus-$corpus.zip" -d $CORPUS_DIR
	rm "$CORPUS_DIR/corpus-$corpus.zip"
done
