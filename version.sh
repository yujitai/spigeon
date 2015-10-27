#!/bin/sh

# usage: ./version.sh
#
# [Environment Variable]
# SCMPF_MODULE_VERSION: exported on scm.baidu.com
# GIT_COMMIT: should exported in git2svn hook

if [[ -z ${SCMPF_MODULE_VERSION} ]]; then
    version="build_$(whoami)@$(hostname)"
else
    version="release_${SCMPF_MODULE_VERSION}"
fi

git_commit=$(git rev-parse --short HEAD 2>/dev/null || echo '')
#git_commit=`(git show-ref --head --hash=8 2> /dev/null || echo '') | head -n1`
if [[ -z "${git_commit}" && -f ./GIT_COMMIT ]]; then
    git_commit=$(cat ./GIT_COMMIT)
fi
if [[ -z "${git_commit}" ]]; then
    git_commit="0000000"
fi

date=$(date +%Y%m%d)

echo "${version}_${git_commit}_${date}"

