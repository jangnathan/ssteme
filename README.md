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
ssteme cache
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

## configuration (in ssteme.cfg)
```
src="main.c","src/"
out="program_name"
```

after ANY file structure update or a new file
Or a config file change, you must cache them like this
```
ssteme cache
```
