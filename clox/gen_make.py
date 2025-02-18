import os

from pathlib import Path
from collections import deque

ROOT_DIR = Path(os.getcwd())
TEMPLATE = """# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -O3
LDFLAGS =

# Targets
TARGET = {target}
OBJS = {objs}

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
{depends}

# Clean up build artifacts
clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: all clean"""


def find_includes(path: str):
    p = Path(path)

    wd = p.parent
    name = p.name.split(".")[0]

    header_file = os.path.join(wd, f"{name}.h")
    c_file = os.path.join(wd, f"{name}.c")

    if not os.path.exists(c_file):
        return None, []

    cwd = wd
    prefixes = deque([])
    while cwd != ROOT_DIR:
        prefixes.appendleft(cwd.name)
        cwd = cwd.parent

    prefix = "/".join(prefixes)

    def visit(path: str):
        includes = []
        with open(path, "r") as file:
            lines = list(map(str.strip, file.readlines()))

        for line in lines:
            if not line.startswith("#include"):
                continue
            if not line[-1] == '"':
                continue

            _header = line[line.find('"') + 1 : -1]

            if not _header.startswith(".."):
                includes.append("/".join([prefix, _header]).strip("/"))
                continue

            q = prefixes.copy()
            _comps = deque(_header.split("/"))
            while _comps[0] == "..":
                q.pop()
                _comps.popleft()

            p = "/".join(q)
            includes.append("/".join([p, *_comps]).strip("/"))

        return includes

    c_includes = visit(c_file)
    h_includes = visit(header_file) if os.path.exists(header_file) else []

    return (
        "/".join([prefix, f"{name}.o"]).strip("/"),
        c_includes + h_includes + ["/".join([prefix, f"{name}.c"]).strip("/")],
    )


def generate_makefile(exe: str, exe_target: str):
    objs, depends = set(), {}

    target, includes = find_includes(exe)
    if target is None:
        return

    objs.add(target)
    depends.update({target: includes})

    visit = set()
    includes = deque(includes)

    while includes:
        cur_visit = includes.popleft()
        if cur_visit in visit:
            continue
        visit.add(cur_visit)

        target, sub_includes = find_includes(os.path.join(str(ROOT_DIR), cur_visit))
        if target is None:
            continue

        objs.add(target)
        depends.update({target: sub_includes})

        for incl in sub_includes:
            if incl not in visit:
                includes.append(incl)

    template = TEMPLATE.format(
        target=exe_target,
        objs=" ".join(objs),
        depends="\n".join([f"{k}: {' '.join(v)}" for k, v in depends.items()]),
    )

    with open("makefile", "w+") as file:
        file.write(template)


generate_makefile(os.path.join(str(ROOT_DIR), "main.c"), "main")
