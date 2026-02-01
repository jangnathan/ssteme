# ssteme
a simple build system for c projects

<br>

this program itself has no build system itself, so you must compile directly

<br>

```
gcc main.c config.c cache.c parse.c -o ssteme
```

write the path onto your .zshrc

## features
- builds and compiles
- can save compilation by compiling only specific files and linking
LATER:
macro support
windows support??

works on linux and mac

## instructions
```
ssteme compile
ssteme link
```
OR
```
ssteme build
```
hydration:
```
ssteme hydrate main.c
```

configuration:
```
src="main.c","src/"
out="program_name"
```
