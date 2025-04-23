# Subgraph Planarizer Executable

Fork of the whole OGDF repo, since that was the easiest option.
(Tsk tsk tsk, imagine being a real programming language and not having a package manager by default in 2025.)

Steps to build on windows
```
git clone git@github.com:stefnotch/ogdf.git
cd ogdf
```

Make sure that CMake is installed (see: Visual Studio Installer)
Open the "developer command prompt"

```
cmake .
MSBuild.exe .\ex-subgraph-planarizer.vcxproj
```

(On Linux, it's `make ex-subgraph-planarizer`)

That compiles the `doc/examples/basic/subgraph-planarizer.cpp` example 

It is then found at `doc/examples/basic/Debug/ex-subgraph-planarizer.exe`

Takes an input.gml and prints which edges need to be deleted. Zero based indices.

For testing, I run it via
```
~\git\ogdf\doc\examples\basic> ./Debug/ex-subgraph-planarizer.exe 
```
