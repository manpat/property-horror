#!/bin/bash

set -euo pipefail

src_dir="src"
build_dir="output"

flags="-std=c++20 -Wall -Wextra -pedantic "
flags+="$(pkg-config --cflags fmt)"

link_libs="$(pkg-config --libs --static fmt)"

object_files=""

function compiler {
	clang++ $flags -xc++ $*
}
function link {
	clang++ $* $link_libs
}

function compile_source {
	local filename="$1"
	shift

	local src="$src_dir/$filename"
	local obj="$build_dir/$filename.o"

	mkdir -p "$(dirname "$obj")"

	if [[ ! -e "$obj" || "$src" -nt "$obj" ]]; then
		echo "building $filename..."
		compiler -c "$src" -o "$obj" $*
	else
		echo "$filename up to date. skipping..."
	fi

	object_files+="$obj "
}


function cmd_build {
	mkdir -p $build_dir

	compile_source main.cpp
	compile_source property.cpp
	link $object_files -ooutput/build
}

function cmd_run {
	./output/build
}


case "${1:-build-run}" in
	"run") cmd_run ;;
	"build") cmd_build ;;

	"build-run")
		cmd_build
		cmd_run
	;;
esac
