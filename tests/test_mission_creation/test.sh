#! /bin/bash
# Usage: test.sh filename
#   creates filename.txt (call graph)
#   creates filename.png (visualized call graph)
TARGET="$HOME/$1"
cp tests/test_mission_creation/main.cpp source/main.cpp
scons -j8 mode=profile && ./endless-sky
gprof endless-sky > "$TARGET".txt
ROOT_NODE="ConditionSet::Test(std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const, int> > > const&) const"
$HOME/projects/gprof2dot/gprof2dot.py -w "$TARGET".txt -z "$ROOT_NODE" -n0 -e0 | dot -Tpng -o "$TARGET".png
git checkout -- source/main.cpp
echo "complete"
