#! /bin/bash
# Usage: test.sh filename
#   creates filename.txt (call graph)
#   creates filename.png (visualized call graph)
TARGET="$HOME/$1"
cp tests/test_planet_accessible/main.cpp source/main.cpp
scons -j8 mode=profile && ./endless-sky
gprof endless-sky > "$TARGET".txt

ROOT_NODE="Planet::IsAccessible(Ship const*) const"
LEAF_NODE="Planet::IsAccessible(Ship const*) const"
$HOME/projects/gprof2dot/gprof2dot.py -w "$TARGET".txt -z "$ROOT_NODE" -n0.1 -e0.1 | dot -Tpng -o "$TARGET".png
$HOME/projects/gprof2dot/gprof2dot.py -w "$TARGET".txt -l "$LEAF_NODE" -n0 -e0 | dot -Tpng -o "$TARGET"_leaf.png

git checkout -- source/main.cpp
echo "complete"
