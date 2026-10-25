#include "vfs.h"
#include "heap.h"
#include "string.h"
#include "vga.h"

static vfs_node_t *root;

static vfs_node_t *make_node(const char *name, vfs_type_t type) {
    vfs_node_t *n = kcalloc(1, sizeof(vfs_node_t));
    if (!n) return NULL;
    strncpy(n->name, name, VFS_NAME_MAX - 1);
    n->type = type;
    return n;
}

static void add_child(vfs_node_t *parent, vfs_node_t *child) {
    child->parent = parent;
    child->sibling = parent->child;
    parent->child = child;
}

static vfs_node_t *find_child(vfs_node_t *dir, const char *name) {
    for (vfs_node_t *c = dir->child; c; c = c->sibling)
        if (strcmp(c->name, name) == 0) return c;
    return NULL;
}

static void seed_file(vfs_node_t *dir, const char *name, const char *text) {
    vfs_node_t *f = make_node(name, VFS_FILE);
    if (!f) return;
    size_t len = strlen(text);
    f->capacity = len + 1;
    f->data = kmalloc(f->capacity);
    if (!f->data) { kfree(f); return; }
    memcpy(f->data, text, len);
    f->data[len] = '\0';
    f->size = len;
    add_child(dir, f);
}

void vfs_init(void) {
    root = make_node("", VFS_DIR);
    vfs_node_t *bin = make_node("bin", VFS_DIR);
    vfs_node_t *etc = make_node("etc", VFS_DIR);
    vfs_node_t *home = make_node("home", VFS_DIR);
    vfs_node_t *tmp = make_node("tmp", VFS_DIR);
    vfs_node_t *usr = make_node("usr", VFS_DIR);
    vfs_node_t *var = make_node("var", VFS_DIR);
    vfs_node_t *dev = make_node("dev", VFS_DIR);
    add_child(root, bin);
    add_child(root, etc);
    add_child(root, home);
    add_child(root, tmp);
    add_child(root, usr);
    add_child(root, var);
    add_child(root, dev);

    vfs_node_t *user = make_node("spark", VFS_DIR);
    add_child(home, user);

    seed_file(etc, "motd",
        "Welcome to SparkOS 1.0\n"
        "A complete hobby operating system.\n"
        "Type 'help' to explore.\n");
    seed_file(etc, "hostname", "sparkos\n");
    seed_file(etc, "version", "SparkOS 1.0.0 (x86)\n");
    seed_file(etc, "passwd", "root:x:0:0:root:/root:/bin/sh\nspark:x:1000:1000:Spark User:/home/spark:/bin/sh\n");
    seed_file(user, "readme.txt",
        "This is your home directory.\n"
        "Try: ls / , cat /etc/motd , free , ps , edit notes.txt\n");
    seed_file(user, "notes.txt", "TODO:\n- learn kernel hacking\n- write a driver\n- have fun\n");
    seed_file(tmp, "hello.txt", "Hello from the SparkOS ramdisk!\n");
    seed_file(bin, "README", "Built-in commands live in the shell, not as ELF binaries (yet).\n");
    seed_file(dev, "null", "");
    seed_file(dev, "zero", "");
    seed_file(var, "log", "boot: SparkOS started\n");
}

vfs_node_t *vfs_root(void) { return root; }

int vfs_is_dir(vfs_node_t *n) { return n && n->type == VFS_DIR; }

void vfs_normalize(const char *cwd, const char *path, char *out, size_t out_sz) {
    char tmp[VFS_PATH_MAX];
    if (!path || !path[0]) {
        strncpy(out, cwd ? cwd : "/", out_sz - 1);
        out[out_sz - 1] = 0;
        return;
    }
    if (path[0] == '/') strncpy(tmp, path, sizeof(tmp) - 1);
    else {
        strncpy(tmp, cwd && cwd[0] ? cwd : "/", sizeof(tmp) - 1);
        size_t n = strlen(tmp);
        if (n == 0 || tmp[n - 1] != '/') {
            if (n + 1 < sizeof(tmp)) { tmp[n] = '/'; tmp[n + 1] = 0; }
        }
        strncat(tmp, path, sizeof(tmp) - strlen(tmp) - 1);
    }
    tmp[sizeof(tmp) - 1] = 0;

    /* Resolve . and .. */
    char *parts[64];
    int nparts = 0;
    char buf[VFS_PATH_MAX];
    strncpy(buf, tmp, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    char *save = buf;
    while (*save == '/') save++;
    while (*save && nparts < 64) {
        char *slash = strchr(save, '/');
        if (slash) *slash = 0;
        if (strcmp(save, ".") == 0) {
            /* skip */
        } else if (strcmp(save, "..") == 0) {
            if (nparts > 0) nparts--;
        } else if (save[0]) {
            parts[nparts++] = save;
        }
        if (!slash) break;
        save = slash + 1;
    }
    out[0] = '/';
    out[1] = 0;
    for (int i = 0; i < nparts; i++) {
        if (strlen(out) > 1) strncat(out, "/", out_sz - strlen(out) - 1);
        strncat(out, parts[i], out_sz - strlen(out) - 1);
    }
    if (nparts == 0) {
        out[0] = '/';
        out[1] = 0;
    }
}

static vfs_node_t *walk(const char *abs) {
    if (!abs || abs[0] != '/') return NULL;
    if (strcmp(abs, "/") == 0) return root;
    vfs_node_t *cur = root;
    char buf[VFS_PATH_MAX];
    strncpy(buf, abs + 1, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    char *save = buf;
    while (*save) {
        char *slash = strchr(save, '/');
        if (slash) *slash = 0;
        cur = find_child(cur, save);
        if (!cur) return NULL;
        if (!slash) break;
        save = slash + 1;
        if (!vfs_is_dir(cur) && *save) return NULL;
    }
    return cur;
}

vfs_node_t *vfs_resolve(const char *cwd, const char *path) {
    char abs[VFS_PATH_MAX];
    vfs_normalize(cwd, path, abs, sizeof(abs));
    return walk(abs);
}

static int split_parent(const char *cwd, const char *path, char *parent, char *name) {
    char abs[VFS_PATH_MAX];
    vfs_normalize(cwd, path, abs, sizeof(abs));
    if (strcmp(abs, "/") == 0) return -1;
    char *slash = abs;
    char *last = abs;
    while (*slash) {
        if (*slash == '/') last = slash;
        slash++;
    }
    if (last == abs) {
        strcpy(parent, "/");
        strcpy(name, abs + 1);
    } else {
        *last = 0;
        strcpy(parent, abs[0] ? abs : "/");
        strcpy(name, last + 1);
    }
    return 0;
}

vfs_node_t *vfs_mkdir(const char *cwd, const char *path) {
    char parent[VFS_PATH_MAX], name[VFS_NAME_MAX];
    if (split_parent(cwd, path, parent, name) < 0) return NULL;
    if (!name[0] || strchr(name, '/')) return NULL;
    vfs_node_t *p = walk(parent);
    if (!p || !vfs_is_dir(p)) return NULL;
    if (find_child(p, name)) return NULL;
    vfs_node_t *d = make_node(name, VFS_DIR);
    if (!d) return NULL;
    add_child(p, d);
    return d;
}

vfs_node_t *vfs_create(const char *cwd, const char *path) {
    char parent[VFS_PATH_MAX], name[VFS_NAME_MAX];
    if (split_parent(cwd, path, parent, name) < 0) return NULL;
    vfs_node_t *p = walk(parent);
    if (!p || !vfs_is_dir(p)) return NULL;
    vfs_node_t *exist = find_child(p, name);
    if (exist) return exist->type == VFS_FILE ? exist : NULL;
    vfs_node_t *f = make_node(name, VFS_FILE);
    if (!f) return NULL;
    f->capacity = 64;
    f->data = kcalloc(1, f->capacity);
    f->size = 0;
    add_child(p, f);
    return f;
}

int vfs_write(vfs_node_t *node, const void *data, size_t len, bool append) {
    if (!node || node->type != VFS_FILE) return -1;
    size_t start = append ? node->size : 0;
    size_t need = start + len + 1;
    if (need > node->capacity) {
        size_t cap = need < 128 ? 128 : need * 2;
        if (cap > VFS_FILE_MAX) cap = VFS_FILE_MAX;
        if (need > cap) return -1;
        uint8_t *nd = kmalloc(cap);
        if (!nd) return -1;
        if (node->data) {
            memcpy(nd, node->data, node->size);
            kfree(node->data);
        }
        node->data = nd;
        node->capacity = cap;
    }
    if (!append) node->size = 0;
    memcpy(node->data + start, data, len);
    node->size = start + len;
    node->data[node->size] = 0;
    return (int)len;
}

ssize_t vfs_read(vfs_node_t *node, size_t offset, void *buf, size_t len) {
    if (!node || node->type != VFS_FILE) return -1;
    if (offset >= node->size) return 0;
    size_t n = node->size - offset;
    if (n > len) n = len;
    memcpy(buf, node->data + offset, n);
    return (ssize_t)n;
}

int vfs_remove(const char *cwd, const char *path) {
    char abs[VFS_PATH_MAX];
    vfs_normalize(cwd, path, abs, sizeof(abs));
    vfs_node_t *n = walk(abs);
    if (!n || n == root) return -1;
    if (n->type == VFS_DIR && n->child) return -1;
    vfs_node_t *p = n->parent;
    if (!p) return -1;
    if (p->child == n) p->child = n->sibling;
    else {
        for (vfs_node_t *c = p->child; c; c = c->sibling) {
            if (c->sibling == n) { c->sibling = n->sibling; break; }
        }
    }
    if (n->data) kfree(n->data);
    kfree(n);
    return 0;
}

void vfs_list(vfs_node_t *dir) {
    if (!dir) { vga_writeln("ls: not found"); return; }
    if (!vfs_is_dir(dir)) { vga_writeln(dir->name); return; }
    for (vfs_node_t *c = dir->child; c; c = c->sibling) {
        vga_set_color(c->type == VFS_DIR ? VGA_LIGHT_BLUE : VGA_LIGHT_GREY, VGA_BLACK);
        vga_write(c->type == VFS_DIR ? "d " : "f ");
        vga_write(c->name);
        if (c->type == VFS_FILE) {
            vga_set_color(VGA_DARK_GREY, VGA_BLACK);
            vga_write("  ");
            vga_write_dec((uint32_t)c->size);
            vga_write(" B");
        }
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_putc('\n');
    }
}
