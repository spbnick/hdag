/**
 * Hash DAG file operations
 */

#include <hdag/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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

hdag_res
hdag_file_create(struct hdag_file *pfile,
                 const char *pathname,
                 int template_sfxlen,
                 mode_t open_mode,
                 uint16_t hash_len,
                 const struct hdag_node *nodes,
                 const uint32_t *node_fanout,
                 const struct hdag_edge *extra_edges,
                 uint32_t extra_edge_num,
                 const uint8_t *unknown_hashes,
                 uint32_t unknown_hash_num)
{
    hdag_res res = HDAG_RES_INVALID;
    int orig_errno;
    int fd = -1;
    struct hdag_file file = HDAG_FILE_CLOSED;
    struct hdag_file_header header = {
        .signature = HDAG_FILE_SIGNATURE,
        .version = {0, 0},
        .hash_len = hash_len,
        .extra_edge_num = extra_edge_num,
        .unknown_hash_num = unknown_hash_num,
    };

    if (pathname != NULL) {
        file.pathname = strdup(pathname);
        if (file.pathname == NULL) {
            goto cleanup;
        }
    }

    /* Copy the fanout (and thus node number) */
    memcpy(header.node_fanout, node_fanout, sizeof(header.node_fanout));

    /* Calculate the file size */
    file.size = hdag_file_size(header.hash_len,
                               header.node_num,
                               header.extra_edge_num,
                               header.unknown_hash_num);

    /* If creating an anonymous mapping */
    if (file.pathname == NULL) {
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
            (void)ptemplate;

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

    /* Close the file (if open) as we're mapped now */
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    /* Initialize the file */
    *(file.header = file.contents) = header;
    file.nodes = (struct hdag_node *)(file.header + 1);
    file.extra_edges = (struct hdag_edge *)(
        (uint8_t *)file.nodes +
        hdag_node_size(file.header->hash_len) * file.header->node_num
    );
    file.unknown_hashes =
        (uint8_t *)file.extra_edges +
        sizeof(struct hdag_edge) * file.header->extra_edge_num;

    /* Copy the data */
    memcpy(file.nodes, nodes,
           hdag_node_size(file.header->hash_len) * file.header->node_num);
    memcpy(file.extra_edges, extra_edges,
           sizeof(struct hdag_edge) * file.header->extra_edge_num);
    memcpy(file.unknown_hashes, unknown_hashes,
           file.header->hash_len * file.header->unknown_hash_num);

    /* The file state should be valid now */
    assert(hdag_file_is_valid(&file));

    /* Hand over the opened file, if requested */
    if (pfile != NULL) {
        *pfile = file;
        file = HDAG_FILE_CLOSED;
    }

    res = HDAG_RES_OK;

cleanup:
    orig_errno = errno;
    if (fd >= 0) {
        close(fd);
        unlink(file.pathname);
    }
    if (file.contents != NULL) {
        munmap(file.contents, file.size);
    }
    free(file.pathname);
    errno = orig_errno;
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_file_open(struct hdag_file *pfile,
               const char *pathname)
{
    hdag_res res = HDAG_RES_INVALID;
    int orig_errno;
    int fd = -1;
    struct hdag_file file = {0, };
    struct stat stat = {0,};

    assert(pathname != NULL);

    file.pathname = strdup(pathname);
    if (file.pathname == NULL) {
        goto cleanup;
    }

    /* Open the file */
    fd = open(file.pathname, O_RDWR);
    if (fd < 0) {
        goto cleanup;
    }

    /* Get file size */
    if (fstat(fd, &stat) != 0) {
        goto cleanup;
    }
    if (stat.st_size < 0 || (uintmax_t)stat.st_size > SIZE_MAX) {
        errno = EFBIG;
        goto cleanup;
    }
    if (stat.st_size < (off_t)sizeof(struct hdag_file_header)) {
        errno = EINVAL;
        goto cleanup;
    }
    file.size = (size_t)stat.st_size;

    /* Memory-map the file */
    file.contents = hdag_file_mmap(fd, file.size);
    if (file.contents == MAP_FAILED) {
        goto cleanup;
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
            file.header->extra_edge_num,
            file.header->unknown_hash_num
        )
    ) {
        errno = EINVAL;
        goto cleanup;
    }
    file.nodes = (struct hdag_node *)(file.header + 1);
    file.extra_edges = (struct hdag_edge *)(
        (uint8_t *)file.nodes +
        hdag_node_size(file.header->hash_len) * file.header->node_num
    );
    file.unknown_hashes =
        (uint8_t *)file.extra_edges +
        sizeof(struct hdag_edge) * file.header->extra_edge_num;

    /* The file state should be valid now */
    assert(hdag_file_is_valid(&file));

    /* Output the opened file, if requested */
    if (pfile == NULL) {
        HDAG_RES_TRY(hdag_file_close(&file));
    } else {
        *pfile = file;
        file = HDAG_FILE_CLOSED;
    }
    res = HDAG_RES_OK;

cleanup:
    orig_errno = errno;
    if (fd >= 0) {
        close(fd);
    }
    if (file.contents != NULL) {
        munmap(file.contents, file.size);
    }
    free(file.pathname);
    errno = orig_errno;
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_file_close(struct hdag_file *pfile)
{
    hdag_res res = HDAG_RES_INVALID;
    assert(hdag_file_is_valid(pfile));
    if (hdag_file_is_open(pfile)) {
        HDAG_RES_TRY(hdag_file_sync(pfile));
        if (munmap(pfile->contents, pfile->size) < 0) {
            goto cleanup;
        }
        free(pfile->pathname);
        *pfile = HDAG_FILE_CLOSED;
        assert(hdag_file_is_valid(pfile));
        assert(!hdag_file_is_open(pfile));
    }
    res = HDAG_RES_OK;
cleanup:
    return HDAG_RES_ERRNO_IF_INVALID(res);
}
