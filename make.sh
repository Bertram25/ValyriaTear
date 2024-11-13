#!/usr/bin/env bash

function priv_clippit
(
    cat <<EOF
Usage: bash ${0} [OPTIONS]
Options:
    build   Build program
EOF
)

function priv_build
(
    if ! (command -v cppcheck); then
        source '/etc/os-release'
        case ${ID:?} in
            debian | ubuntu)
                sudo apt-get update
                sudo apt-get install -y cppcheck {freeglut3,lib{openal,alut,vorbis,ogg,lua5.1,gettextpo,boost-all,qt4-opengl,glew,sdl2{-image,-ttf,}}}-dev
                ;;
        esac
    fi
    git submodule update --recursive --init
    git submodule update --recursive --remote
    cmake "${PWD}"
    make
)

function priv_main
(
    set -euo pipefail
    if ((${#})); then
        case ${1} in
            build) priv_build ;;
            *) priv_clippit ;;
        esac
    else
        priv_clippit
    fi
)

priv_main "${@}" >/dev/null
