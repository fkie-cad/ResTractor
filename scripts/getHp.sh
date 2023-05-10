#!/bin/bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT="$(dirname "$ROOT")"

inc_dir="${ROOT}/res/inc"
lib_dir="${ROOT}/res/lib"
tmp_dir="${ROOT}/tmp"
repo_url=https://github.com/fkie-cad/headerParser.git
repo_name=headerParser
bin_name=libheaderparser
bin_ext=.a
build_path=build
build_cmd=linuxBuild.sh

name=headerParser
pos_targets="st|cln|del"
target="st"
build_mode=2
mode="Release"
help=0
debug_print=2
error_print=0

# Clean tmp directory fro
#
# @param $1 build directory
function cleanUp() {
    target=${tmp_dir}
    echo "cleaning up tmp files: ${target}"

    if [[ ${target} == "${ROOT}" ]]; then
        return
    fi

    cd "${ROOT}" || return 1

    rm -rf ${target} 2> /dev/null

    return 0
}

# Delete build directory and all files in it
#
# @param $1 build directory
function delete() {
    local dir=$1

    echo "deleting dir: $dir"

    if [[ ${dir} == "${ROOT}" ]]; then
        return 0
    fi

    rm -rf ${dir} 2> /dev/null

    return 0
}

# CMake build a target
#
# @param $3 build mode
function buildHpLib() {
    local dir=$1
    local mode=$2
    local dp=$3

    local m=-r

    cd ${dir}

    if [[ ${mode} == "Debug" || ${mode} == "debug" ]] ; then
        m=-d
    fi

    ./${build_cmd} -t st ${m} -p ${dp}

    return 0
}

# CMake build a target
#
# @param $3 build mode
function copyHpLib() {
    local repo_dir=$1
    local inc_dir=$2
    local lib_dir=$3
    local mode=$4

    local build_dir=${repo_dir}
    local src_dir=${repo_dir}/src

    if [[ ${mode} == "Debug" || ${mode} == "debug" ]]; then
        build_dir="${repo_dir}/build/debug"
    else
        build_dir="${repo_dir}/build"
    fi

    if ! mkdir -p "${lib_dir}"; then
        return -1
    fi

    cp "${build_dir}/${bin_name}${bin_ext}" ${lib_dir}


    if ! mkdir -p "${inc_dir}/pe"; then
        return -1
    fi

    local files=(exp.h Globals.h HeaderData.h headerParserLib.h headerParserLibPE.h PEHeaderData.h pe/PEHeader.h pe/PEHeaderOffsets.h)
    local end=${#files[@]}
    for ((i = 0; i < end; i++)); do
        cp "${src_dir}/${files[$i]}" "${inc_dir}/${files[$i]}"
    done

    return 0
}

# Clone headerParser repo
#
# @param $1 url
# @param $2 name
function cloneRepo()
{
    local url=$1
    local name=$2
    local tmp_dir=$3


    if ! mkdir ${tmp_dir}; then
        return -1
    fi

    cd "${tmp_dir}" || return -2

    if ! git clone ${url}; then
        return -3
    fi

    cd - || return -4

    return 0
}

function printUsage() {
    echo "Usage: $0 [-m Debug|Release] [-h]"
    echo "Default: $0 [-r]"
    return 0;
}

function printHelp() {
    printUsage
    echo ""
    echo "-d Build in debug mode"
    echo "-r Build in release mode"
    echo "-h Print this."
    return 0;
}

while getopts ":p:drh" opt; do
    case $opt in
    h)
        help=1
        ;;
    d)
        build_mode=1
        ;;
    p)
        debug_print="$OPTARG"
        ;;
    r)
        build_mode=2
        ;;
    \?)
        echo "Invalid option -$OPTARG" >&2
        ;;
    esac
done

if [[ ${help} == 1 ]]; then
    printHelp
    exit $?
fi

if [[ $((build_mode & 2)) == 2 ]]; then
    mode="Release"
    build_dir=${release_build_dir}
else
    mode="Debug"
    build_dir=${debug_build_dir}
fi

echo "target: "${target}
echo "mode: "${mode}
echo "build_dir: "${build_dir}

if [[ ${target} == "cln" || ${target} == "clean" ]]; then
    clean ${build_dir}
    exit $?
elif [[ ${target} == "del" || ${target} == "delete" ]]; then
    delete ${build_dir}
    exit $?
else
    if [[ ${target} == "st" ]]; then
        target=${name}_st
    else
        echo "Unknown target: ${target}"
        exit $?
    fi

    cloneRepo ${repo_url} ${repo_name} ${tmp_dir}
    buildHpLib "${tmp_dir}/${repo_name}" ${mode} ${debug_print}
    copyHpLib "${tmp_dir}/${repo_name}" "${inc_dir}" "${lib_dir}" ${mode}
    cleanUp "${tmp_dir}"

    exit $?
fi

exit $?
