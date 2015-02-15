#include "ftpfs.h"
#include "super.h"
#include "inode.h"

struct ftp_fs_mount_opts {
    umode_t mode;
};

struct ftp_fs_info {
    struct ftp_fs_mount_opts mount_opts;
};

enum {
    Opt_mode, Opt_err
};

static const match_table_t tokens = {
    {Opt_mode, "mode=%o"},
    {Opt_err, NULL}
};

const struct super_operations ftp_fs_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
    .show_options = generic_show_options,
};

static int ftp_fs_parse_options(char *data, struct ftp_fs_mount_opts *opts) {
    substring_t args[MAX_OPT_ARGS];
    int option;
    int token;
    char *p;

    opts->mode = FTP_FS_DEFAULT_MODE;

    while ((p = strsep(&data, ",")) != NULL) {
        if (!*p) continue;

        token = match_token(p, tokens, args);
        switch (token) {
            case Opt_mode:
                if (match_octal(&args[0], &option))
                    return -EINVAL;
                opts->mode = option & S_IALLUGO;
                break;
        }
    }

    return 0;
}

int ftp_fs_fill_super(struct super_block *sb, void *data, int silent) {
    struct ftp_fs_info *fsi;
    struct inode* inode;
    int err;

    pr_debug("begin ftp_fs_fill_super\n");
    save_mount_options(sb, data);

    fsi = kzalloc(sizeof(struct ftp_fs_info), GFP_KERNEL);
    sb->s_fs_info = fsi;
    if (!fsi) return -ENOMEM;

    err = ftp_fs_parse_options(data, &fsi->mount_opts);
    if (err) return err;

    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_magic = FTP_FS_MAGIC;
    sb->s_op = &ftp_fs_ops;
    sb->s_time_gran = 1;

    pr_debug("try to fetch a inode to store super block\n");
    inode = ftp_fs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
    sb->s_root = d_make_root(inode);
    if (!sb->s_root) return -ENOMEM;

    return 0;
}

static void ftp_fs_kill_sb(struct super_block *sb) {
    kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

struct dentry* ftp_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    return mount_nodev(fs_type, flags, data, ftp_fs_fill_super);
}

struct file_system_type ftp_fs_type = {
    .name = "ftpfs",
    .mount = ftp_fs_mount,
    .kill_sb = ftp_fs_kill_sb,
    .fs_flags = FS_USERNS_MOUNT,
};

