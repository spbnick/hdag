/*
 * Hash DAG file operations
 */

#include <hdag/bundle.h>
#include <hdag/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>

/**
 * Memory-map the contents of a hash DAG file.
 *
 * @param fd        The descriptor of the file to memory-map, or a negative
 *                  number to create an anonymous mapping.
 * @param size      The length of the area to memory-map.
 *
 * @return The pointer to the mapped file, or MAP_FAILED in case of failure.
 *         The errno is set on failure.
 */
static void *
hdag_file_mmap(int fd, size_t size)
{
    return mmap(NULL, size, PROT_READ | PROT_WRITE,
                MAP_SHARED | (fd < 0 ? MAP_ANONYMOUS : 0),
                fd, 0);
}

bool
hdag_file_create(struct hdag_file *pfile,
                 const char *pathname,
                 int template_sfxlen,
                 mode_t open_mode,
                 uint16_t hash_len,
                 struct hdag_node_seq node_seq)
{
    bool result = false;
    int orig_errno;
    int fd = -1;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(hash_len);
    struct hdag_file file = HDAG_FILE_CLOSED;
    struct hdag_file_header header = {
        .signature = HDAG_FILE_SIGNATURE,
        .version = {0, 0},
        .hash_len = hash_len,
    };

    assert(pathname != NULL);
    assert(strlen(pathname) < sizeof(file.pathname));
    assert(hdag_hash_len_is_valid(hash_len));

    /* Load the nodes and their targets into a bundle */
    if (!hdag_bundle_load_node_seq(&bundle, node_seq)) {
        goto cleanup;
    }

    strncpy(file.pathname, pathname, sizeof(file.pathname));

    /* Calculate the file size */
    file.size = hdag_file_size(hash_len, bundle.nodes.slots_occupied,
                               bundle.extra_edges.slots_occupied);

    /* If creating an anonymous mapping */
    if (*file.pathname == 0) {
        fd = -1;
    /* Else, mapping a file */
    } else {
        /* If creating a "temporary" file */
        if (template_sfxlen >= 0) {
            const char template[] = "XXXXXX";
            const char *ptemplate;
            ptemplate = strstr(file.pathname, template);
            assert(ptemplate != NULL);
            assert(template_sfxlen <=
                   (int)strlen(file.pathname) - (int)strlen(template));
            assert(ptemplate == file.pathname +
                   (strlen(file.pathname) -
                    strlen(template) -
                    template_sfxlen));

            fd = mkstemps(file.pathname, template_sfxlen);
            if (fd < 0) {
                goto cleanup;
            }
            if (fchmod(fd, open_mode) < 0) {
                goto cleanup;
            }
        /* Else, if creating a literal, "non-temporary" file */
        } else {
            fd = open(file.pathname, O_RDWR | O_CREAT | O_EXCL, open_mode);
            if (fd < 0) {
                goto cleanup;
            }
        }

        /* Expand the file to our size (filling it with zeros) */
        if (ftruncate(fd, file.size) < 0) {
            goto cleanup;
        }
    }

    /* Memory-map the file */
    file.contents = hdag_file_mmap(fd, file.size);
    if (file.contents == MAP_FAILED) {
        goto cleanup;
    }

    /* Close the file as we're mapped now */
    close(fd);
    fd = -1;

    /* Initialize the file */
    *(file.header = file.contents) = header;
    file.nodes = (struct hdag_node *)(file.header + 1);
    file.extra_edges = (struct hdag_edge *)(
            (uint8_t *)file.nodes +
            hdag_node_size(file.header->hash_len) *
            file.header->node_num
    );

    /* Copy the bundle data */
    file.header->node_num = bundle.nodes.slots_occupied;
    memcpy(file.nodes, bundle.nodes.slots,
           hdag_darr_occupied_size(&bundle.nodes));
    file.header->extra_edge_num = bundle.extra_edges.slots_occupied;
    memcpy(file.extra_edges, bundle.extra_edges.slots,
           hdag_darr_occupied_size(&bundle.extra_edges));

    /* The file state should be valid now */
    assert(hdag_file_is_valid(&file));

    /* Hand over the opened file, if requested */
    if (pfile != NULL) {
        *pfile = file;
        file = HDAG_FILE_CLOSED;
    }

    result = true;

cleanup:
    orig_errno = errno;
    if (fd >= 0) {
        close(fd);
        unlink(file.pathname);
    }
    if (file.contents != NULL) {
        munmap(file.contents, file.size);
    }
    hdag_bundle_cleanup(&bundle);
    errno = orig_errno;
    return result;
}

bool
hdag_file_open(struct hdag_file *pfile,
               const char *pathname)
{
    int orig_errno;
    int fd = -1;
    struct hdag_file file = {0, };
    struct stat stat = {0,};

    assert(pathname != NULL);
    assert(*pathname != '\0');
    assert(strlen(pathname) < sizeof(file.pathname));

    strncpy(file.pathname, pathname, sizeof(file.pathname));

    /* Open the file */
    fd = open(file.pathname, O_RDWR);
    if (fd < 0) {
        goto fail;
    }

    /* Get file size */
    if (fstat(fd, &stat) != 0) {
        goto fail;
    }
    if (stat.st_size < 0 || (uintmax_t)stat.st_size > SIZE_MAX) {
        errno = EFBIG;
        goto fail;
    }
    if (stat.st_size < (off_t)sizeof(struct hdag_file_header)) {
        errno = EINVAL;
        goto fail;
    }
    file.size = (size_t)stat.st_size;

    /* Memory-map the file */
    file.contents = hdag_file_mmap(fd, file.size);
    if (file.contents == MAP_FAILED) {
        goto fail;
    }

    /* Close the file as we're mapped now */
    close(fd);
    fd = -1;

    /* Parse the file headers */
    file.header = file.contents;
    if (
        !hdag_file_header_is_valid(file.header) ||
        file.size != hdag_file_size(
            file.header->hash_len,
            file.header->node_num,
            file.header->extra_edge_num
        )
    ) {
        errno = EINVAL;
        goto fail;
    }
    file.nodes = (struct hdag_node *)(file.header + 1);
    file.extra_edges = (struct hdag_edge *)(
            (uint8_t *)file.nodes +
            hdag_node_size(file.header->hash_len) *
            file.header->node_num
    );

    /* The file state should be valid now */
    assert(hdag_file_is_valid(&file));

    /* Output the opened file, if requested */
    if (pfile == NULL) {
        hdag_file_close(&file);
    } else {
        *pfile = file;
    }
    return true;

fail:
    orig_errno = errno;
    if (fd >= 0) {
        close(fd);
    }
    if (file.contents != NULL) {
        munmap(file.contents, file.size);
    }
    errno = orig_errno;
    return false;
}

bool
hdag_file_close(struct hdag_file *pfile)
{
    assert(hdag_file_is_valid(pfile));
    if (hdag_file_is_open(pfile)) {
        hdag_file_sync(pfile);
        if (munmap(pfile->contents, pfile->size) < 0) {
            return false;
        }
        *pfile = HDAG_FILE_CLOSED;
        assert(hdag_file_is_valid(pfile));
        assert(!hdag_file_is_open(pfile));
    }
    return true;
}
