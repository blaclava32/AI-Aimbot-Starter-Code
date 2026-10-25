#ifndef SPARK_VFS_H
#define SPARK_VFS_H

#include "types.h"

#define VFS_NAME_MAX 48
#define VFS_PATH_MAX 128
#define VFS_FILE_MAX 8192

typedef enum {
    VFS_FILE = 1,
    VFS_DIR  = 2
} vfs_type_t;

typedef struct vfs_node {
    char name[VFS_NAME_MAX];
    vfs_type_t type;
    uint8_t *data;
    size_t size;
    size_t capacity;
    struct vfs_node *child;
    struct vfs_node *sibling;
    struct vfs_node *parent;
} vfs_node_t;

void vfs_init(void);
vfs_node_t *vfs_root(void);
vfs_node_t *vfs_resolve(const char *cwd, const char *path);
vfs_node_t *vfs_mkdir(const char *cwd, const char *path);
vfs_node_t *vfs_create(const char *cwd, const char *path);
int vfs_write(vfs_node_t *node, const void *data, size_t len, bool append);
int vfs_remove(const char *cwd, const char *path);
void vfs_list(vfs_node_t *dir);
void vfs_normalize(const char *cwd, const char *path, char *out, size_t out_sz);
int vfs_is_dir(vfs_node_t *n);
ssize_t vfs_read(vfs_node_t *node, size_t offset, void *buf, size_t len);

#endif
